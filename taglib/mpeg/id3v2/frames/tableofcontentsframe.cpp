/***************************************************************************
    copyright            : (C) 2013 by Lukas Krejci
    email                : krejclu6@fel.cvut.cz
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#include <tbytevectorlist.h>
#include <tpropertymap.h>
#include <tdebug.h>

#include "tableofcontentsframe.h"

using namespace TagLib;
using namespace ID3v2;

class TableOfContentsFrame::TableOfContentsFramePrivate
{
public:
  TableOfContentsFramePrivate() :
    tagHeader(0)
  {
    embeddedFrameList.setAutoDelete(true);
  }

  const ID3v2::Header *tagHeader;
  ByteVector elementID;
  bool isTopLevel;
  bool isOrdered;
  ByteVectorList childElements;
  FrameListMap embeddedFrameListMap;
  FrameList embeddedFrameList;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

TableOfContentsFrame::TableOfContentsFrame(const ID3v2::Header *tagHeader, const ByteVector &data) :
    ID3v2::Frame(data)
{
  d = new TableOfContentsFramePrivate;
  d->tagHeader = tagHeader;
  setData(data);
}

TableOfContentsFrame::TableOfContentsFrame(const ByteVector &eID, const ByteVectorList &ch,
                                           const FrameList &eF) :
    ID3v2::Frame("CTOC")
{
  d = new TableOfContentsFramePrivate;
  d->elementID = eID;
  d->childElements = ch;
  FrameList l = eF;
  for(FrameList::ConstIterator it = l.begin(); it != l.end(); ++it)
    addEmbeddedFrame(*it);
}

TableOfContentsFrame::~TableOfContentsFrame()
{
  delete d;
}

ByteVector TableOfContentsFrame::elementID() const
{
  return d->elementID;
}

bool TableOfContentsFrame::isTopLevel() const
{
  return d->isTopLevel;
}

bool TableOfContentsFrame::isOrdered() const
{
  return d->isOrdered;
}

uint TableOfContentsFrame::entryCount() const
{
  return d->childElements.size();
}

ByteVectorList TableOfContentsFrame::childElements() const
{
  return d->childElements;
}

void TableOfContentsFrame::setElementID(const ByteVector &eID)
{
  d->elementID = eID;
  if(eID.at(eID.size() - 1) != char(0))
    d->elementID.append(char(0));
}

void TableOfContentsFrame::setIsTopLevel(const bool &t)
{
  d->isTopLevel = t;
}

void TableOfContentsFrame::setIsOrdered(const bool &o)
{
  d->isOrdered = o;
}

void TableOfContentsFrame::setChildElements(const ByteVectorList &l)
{
  d->childElements = l;
}

void TableOfContentsFrame::addChildElement(const ByteVector &cE)
{
  d->childElements.append(cE);
}

void TableOfContentsFrame::removeChildElement(const ByteVector &cE)
{
  ByteVectorList::Iterator it = d->childElements.find(cE);
  d->childElements.erase(it);
}

const FrameListMap &TableOfContentsFrame::embeddedFrameListMap() const
{
  return d->embeddedFrameListMap;
}

const FrameList &TableOfContentsFrame::embeddedFrameList() const
{
  return d->embeddedFrameList;
}

const FrameList &TableOfContentsFrame::embeddedFrameList(const ByteVector &frameID) const
{
  return d->embeddedFrameListMap[frameID];
}

void TableOfContentsFrame::addEmbeddedFrame(Frame *frame)
{
  d->embeddedFrameList.append(frame);
  d->embeddedFrameListMap[frame->frameID()].append(frame);
}

void TableOfContentsFrame::removeEmbeddedFrame(Frame *frame, bool del)
{
  // remove the frame from the frame list
  FrameList::Iterator it = d->embeddedFrameList.find(frame);
  d->embeddedFrameList.erase(it);

  // ...and from the frame list map
  it = d->embeddedFrameListMap[frame->frameID()].find(frame);
  d->embeddedFrameListMap[frame->frameID()].erase(it);

  // ...and delete as desired
  if(del)
    delete frame;
}

void TableOfContentsFrame::removeEmbeddedFrames(const ByteVector &id)
{
  FrameList l = d->embeddedFrameListMap[id];
  for(FrameList::ConstIterator it = l.begin(); it != l.end(); ++it)
    removeEmbeddedFrame(*it, true);
}

String TableOfContentsFrame::toString() const
{
  return String::null;
}

PropertyMap TableOfContentsFrame::asProperties() const
{
  PropertyMap map;

  map.unsupportedData().append(frameID() + String("/") + d->elementID);

  return map;
}

TableOfContentsFrame *TableOfContentsFrame::findByElementID(const ID3v2::Tag *tag,
                                                            const ByteVector &eID) // static
{
  ID3v2::FrameList tablesOfContents = tag->frameList("CTOC");

  for(ID3v2::FrameList::ConstIterator it = tablesOfContents.begin();
      it != tablesOfContents.end();
      ++it)
  {
    TableOfContentsFrame *frame = dynamic_cast<TableOfContentsFrame *>(*it);
    if(frame && frame->elementID() == eID)
      return frame;
  }

  return 0;
}

TableOfContentsFrame *TableOfContentsFrame::findTopLevel(const ID3v2::Tag *tag) // static
{
  ID3v2::FrameList tablesOfContents = tag->frameList("CTOC");

  for(ID3v2::FrameList::ConstIterator it = tablesOfContents.begin();
      it != tablesOfContents.end();
      ++it)
  {
    TableOfContentsFrame *frame = dynamic_cast<TableOfContentsFrame *>(*it);
    if(frame && frame->isTopLevel() == true)
      return frame;
  }

  return 0;
}

void TableOfContentsFrame::parseFields(const ByteVector &data)
{
  uint size = data.size();
  if(size < 6) {
    debug("A CTOC frame must contain at least 6 bytes (1 byte element ID terminated by "
          "null, 1 byte flags, 1 byte entry count and 1 byte child element ID terminated "
          "by null.");
    return;
  }

  int pos = 0, embPos = 0;
  d->elementID = readStringField(data, String::Latin1, &pos).data(String::Latin1);
  d->elementID.append(char(0));
  d->isTopLevel = (data.at(pos) & 2) > 0;
  d->isOrdered = (data.at(pos++) & 1) > 0;
  uint entryCount = data.at(pos++);
  for(uint i = 0; i < entryCount; i++)
  {
    ByteVector childElementID = readStringField(data, String::Latin1, &pos).data(String::Latin1);
    childElementID.append(char(0));
    d->childElements.append(childElementID);
  }

  size -= pos;
  while(embPos < size - header()->size()) {
    Frame *frame = FrameFactory::instance()->createFrame(data.mid(pos + embPos), d->tagHeader);

    if(!frame)
      return;

    // Checks to make sure that frame parsed correctly.
    if(frame->size() <= 0) {
      delete frame;
      return;
    }

    embPos += frame->size() + header()->size();
    addEmbeddedFrame(frame);
  }
}

ByteVector TableOfContentsFrame::renderFields() const
{
  ByteVector data;

  data.append(d->elementID);
  char flags = 0;
  if(d->isTopLevel)
    flags += 2;
  if(d->isOrdered)
    flags += 1;
  data.append(flags);
  data.append((char)(entryCount()));
  ByteVectorList::ConstIterator it = d->childElements.begin();
  while(it != d->childElements.end()) {
    data.append(*it);
    it++;
  }
  FrameList l = d->embeddedFrameList;
  for(FrameList::ConstIterator it = l.begin(); it != l.end(); ++it)
    data.append((*it)->render());

  return data;
}

TableOfContentsFrame::TableOfContentsFrame(const ID3v2::Header *tagHeader,
                                           const ByteVector &data, Header *h) :
  Frame(h)
{
  d = new TableOfContentsFramePrivate;
  d->tagHeader = tagHeader;
  parseFields(fieldData(data));
}
