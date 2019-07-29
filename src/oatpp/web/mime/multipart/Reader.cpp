/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include "Reader.hpp"

#include "oatpp/core/data/stream/BufferInputStream.hpp"

namespace oatpp { namespace web { namespace mime { namespace multipart {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// InMemoryParser

InMemoryParser::InMemoryParser(Multipart* multipart)
  : m_multipart(multipart)
{}

void InMemoryParser::onPartHeaders(const Headers& partHeaders) {
  m_currPart = std::make_shared<Part>(partHeaders);
}

void InMemoryParser::onPartData(p_char8 data, oatpp::data::v_io_size size) {
  if(size > 0) {
    m_buffer.write(data, size);
  } else {
    auto fullData = m_buffer.toString();
    m_buffer.clear();
    auto stream = std::make_shared<data::stream::BufferInputStream>(fullData.getPtr(), fullData->getData(), fullData->getSize());
    m_currPart->setDataInfo(stream, fullData, fullData->getSize());
    m_multipart->addPart(m_currPart);
    m_currPart = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AsyncInMemoryParser

AsyncInMemoryParser::AsyncInMemoryParser(Multipart* multipart)
  : m_multipart(multipart)
{}

async::CoroutineStarter AsyncInMemoryParser::onPartHeadersAsync(const Headers& partHeaders) {
  m_currPart = std::make_shared<Part>(partHeaders);
  return nullptr;
}

async::CoroutineStarter AsyncInMemoryParser::onPartDataAsync(p_char8 data, oatpp::data::v_io_size size) {
  if(size > 0) {
    m_buffer.write(data, size);
  } else {
    auto fullData = m_buffer.toString();
    m_buffer.clear();
    auto stream = std::make_shared<data::stream::BufferInputStream>(fullData.getPtr(), fullData->getData(), fullData->getSize());
    m_currPart->setDataInfo(stream, fullData, fullData->getSize());
    m_multipart->addPart(m_currPart);
    m_currPart = nullptr;
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// InMemoryReader

Reader::Reader(Multipart* multipart)
  : m_parser(multipart->getBoundary(), std::make_shared<InMemoryParser>(multipart), nullptr)
{}

data::v_io_size Reader::write(const void *data, data::v_io_size count) {
  m_parser.parseNext((p_char8) data, count);
  return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AsyncReader

AsyncReader::AsyncReader(const std::shared_ptr<Multipart>& multipart)
  : m_parser(multipart->getBoundary(), nullptr, std::make_shared<AsyncInMemoryParser>(multipart.get()))
  , m_multipart(multipart)
{}

oatpp::async::Action AsyncReader::writeAsyncInline(oatpp::async::AbstractCoroutine* coroutine,
                                                   oatpp::data::stream::AsyncInlineWriteData& inlineData,
                                                   oatpp::async::Action&& nextAction)
{
  return m_parser.parseNextAsyncInline(coroutine, inlineData, std::forward<async::Action>(nextAction));
}

}}}}
