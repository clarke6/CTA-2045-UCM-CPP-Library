// =====================================================================================
// Copyright (c) 2016, Electric Power Research Institute (EPRI)
// All rights reserved.
//
// libcea2045 ("this software") is licensed under BSD 3-Clause license.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// *  Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// *  Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// *  Neither the name of EPRI nor the names of its contributors may
//    be used to endorse or promote products derived from this software without
//    specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
// OF SUCH DAMAGE.
//
// This EPRI software incorporates work covered by the following copyright and permission
// notices. You may not use these works except in compliance with their respective
// licenses, which are provided below.
//
// These works are provided by the copyright holders and contributors "as is" and any express or
// implied warranties, including, but not limited to, the implied warranties of merchantability
// and fitness for a particular purpose are disclaimed.
//
// This software relies on the following libraries and licenses:
//
// #########################################################################################
// Boost Software License, Version 1.0
// #########################################################################################
//
// * catch++ v1.2.1 (https://github.com/philsquared/Catch)
//
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// #########################################################################################
// MIT Licence
// #########################################################################################
//
// * easylogging++ Copyright (c) 2017 muflihun.com
//   https://github.com/easylogging/easyloggingpp
//   https://easylogging.muflihun.com
//   https://muflihun.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
/*
 * ProcessMessageUCM.cpp
 *
 *  Created on: Aug 24, 2015
 *      Author: dupes
 */

#include "ProcessMessageUCM.h"
#include "../util/Checksum.h"

namespace cea2045 {

ProcessMessageUCM::ProcessMessageUCM(IUCM *ucm) :
	m_ucm(ucm)
{
}

//======================================================================================

ProcessMessageUCM::~ProcessMessageUCM()
{
}

//======================================================================================

void ProcessMessageUCM::processLinkLayerAckNak(ILinkLayerCommSend *linkLayer, cea2045MessageHeader *message, MessageCode messageCode)
{
	if (message->msgType1 == LINK_LAYER_ACK_MSG_TYP1)
	{
		m_ucm->processAckReceived(messageCode);
	}
	else
	{
		LinkLayerNakCode nak = ConvertEnums::convertLinkLayerNak(message->msgType2);

		m_ucm->processNakReceived(nak, messageCode);
	}
}

//======================================================================================

void ProcessMessageUCM::processMessageTypeSupported(ILinkLayerCommSend *linkLayer, cea2045MessageHeader *message)
{
	unsigned short messageType = *((unsigned short *)(message));

	MessageTypeCode messageTypeCode = ConvertEnums::convertMessageType(messageType);

	bool messageTypeIsSuppported = m_ucm->isMessageTypeSupported(messageTypeCode);

	if (messageTypeIsSuppported)
	{
		linkLayer->sendLinkLayerAck();
	}
	else
	{
		linkLayer->sendLinkLayerNak(LinkLayerNakCode::UNSUPPORTED_MESSAGE_TYPE);
	}
}

//======================================================================================

void ProcessMessageUCM::processBasicMessage(ILinkLayerCommSend *linkLayer, cea2045Basic *basic)
{
	switch (basic->opCode1)
	{
		case APP_ACK:
			m_ucm->processAppAckReceived(basic);

			linkLayer->sendLinkLayerAck();

			break;

		case APP_NAK:
			m_ucm->processAppNakReceived(basic);

			linkLayer->sendLinkLayerAck();

			break;

		case OPER_STATE_RESP:
			m_ucm->processOperationalStateReceived(basic);

			linkLayer->sendLinkLayerAck();

			break;

		case CUST_OVERRIDE:
			m_ucm->processAppCustomerOverride(basic);

			linkLayer->sendLinkLayerAck();

			cea2045Basic response;

			response.msgType1 = BASIC_MSG_TYP1;
			response.msgType2 = BASIC_MSG_TYP2;
			response.length = htobe16(2);
			response.opCode1 = APP_ACK;
			response.opCode2 = CUST_OVERRIDE;

			response.checksum = Checksum::calculate((unsigned char *)&response, sizeof(response) - 2);

			linkLayer->sendResponse((unsigned char*)&response, sizeof(response));

			break;

		default:
			linkLayer->sendLinkLayerNak(LinkLayerNakCode::UNSUPPORTED_MESSAGE_TYPE);
			break;
	}
}

//======================================================================================

void ProcessMessageUCM::processIntermediateMessage(ILinkLayerCommSend *linkLayer, cea2045MessageHeader *message)
{
	if (!m_ucm->isMessageTypeSupported(MessageTypeCode::INTERMEDIATE))
	{
		linkLayer->sendLinkLayerNak(LinkLayerNakCode::REQUEST_NOT_SUPPORTED);
		return;
	}

	cea2045Intermediate *intermediate = (cea2045Intermediate *)message;

	unsigned short intermediateType = *((unsigned short *)(&(intermediate->opCode1)));

	IntermediateTypeCode intermediateTypeCode = ConvertEnums::convertIntermediateType(intermediateType);

	switch (intermediateTypeCode)
	{
		case IntermediateTypeCode::GET_UTC_TIME_REQUEST:
		{
			if (message->getLength() == 2)
			{
				linkLayer->sendLinkLayerAck();

				cea2045GetUTCTimeResponse response;

				response.utcSeconds = 0;
				response.timezoneOffsetQuarterHours = 0;
				response.dstOffsetQuarterHours = 0;

				m_ucm->processGetUTCTimeResponse(&response);

				response.msgType1 = INTERMEDIATE_MSG_TYP1;
				response.msgType2 = INTERMEDIATE_MSG_TYP2;
				response.setLength();
				response.opCode1 = GET_UTC_TIME;
				response.opCode2 = OP_CODE2_REPLY;
				response.responseCode = OP_CODE2_REPLY;

				response.setChecksum();

				linkLayer->sendResponse((unsigned char*)&response, sizeof(response));
			}
			else
			{
				linkLayer->sendLinkLayerNak(LinkLayerNakCode::REQUEST_NOT_SUPPORTED);
			}

			break;
		}

		case IntermediateTypeCode::INFO_RESPONSE:
		{
			cea2045DeviceInfoResponse *infoResponse = (cea2045DeviceInfoResponse *)message;

			m_ucm->processDeviceInfoResponse(infoResponse);

			linkLayer->sendLinkLayerAck();

			break;
		}

		case IntermediateTypeCode::COMMODITY_RESPONSE:
		{
			cea2045CommodityResponse *commodityResponse = (cea2045CommodityResponse *)message;

			m_ucm->processCommodityResponse(commodityResponse);

			linkLayer->sendLinkLayerAck();

			break;
		}

		case IntermediateTypeCode::SET_ENERGY_PRICE_RESPONSE:
		{
			cea2045IntermediateResponse *intermediateResponse = (cea2045IntermediateResponse *)message;

			m_ucm->processSetEnergyPriceResponse(intermediateResponse);

			linkLayer->sendLinkLayerAck();

			break;
		}

		case IntermediateTypeCode::GET_SET_TEMPERATURE_OFFSET_RESPONSE:
		{
			if (message->getLength() + 6 == sizeof(cea2045IntermediateResponse))
			{
				cea2045IntermediateResponse *intermediateResponse = (cea2045IntermediateResponse *)message;

				m_ucm->processSetTemperatureOffsetResponse(intermediateResponse);

			}
			else if (message->getLength() + 6 == sizeof(cea2045GetTemperateOffsetResponse))
			{
				cea2045GetTemperateOffsetResponse *getTemperateOffsetResponse = (cea2045GetTemperateOffsetResponse *)message;

				m_ucm->processGetTemperatureOffsetResponse(getTemperateOffsetResponse);
			}
			else
			{

			}

			linkLayer->sendLinkLayerAck();

			break;
		}

		case IntermediateTypeCode::GET_SET_SETPOINT_RESPONSE:
		{
			if (message->getLength() + 6 == sizeof(cea2045IntermediateResponse))
			{
				cea2045IntermediateResponse *intermediateResponse = (cea2045IntermediateResponse *)message;

				m_ucm->processSetSetpointsResponse(intermediateResponse);

				linkLayer->sendLinkLayerAck();
			}
			else if (message->getLength() + 6 == sizeof(cea2045GetSetpointsResponse2))
			{
				cea2045GetSetpointsResponse2 *setpointsResponse = (cea2045GetSetpointsResponse2 *)message;

				m_ucm->processGetSetpointsResponse(setpointsResponse);

				linkLayer->sendLinkLayerAck();
			}
			else if (message->getLength() + 6 == sizeof(cea2045GetSetpointsResponse1))
			{
				cea2045GetSetpointsResponse1 *setpointsResponse = (cea2045GetSetpointsResponse1 *)message;

				m_ucm->processGetSetpointsResponse(setpointsResponse);

				linkLayer->sendLinkLayerAck();
			}

			break;
		}

		case IntermediateTypeCode::GET_PRESENT_TEMPERATURE_RESPONSE:
		{
			cea2045GetPresentTemperatureResponse *getPresentTemperature = (cea2045GetPresentTemperatureResponse *)message;

			m_ucm->processGetPresentTemperatureResponse(getPresentTemperature);

			linkLayer->sendLinkLayerAck();

			break;
		}

		case IntermediateTypeCode::START_CYCLING_RESPONSE:
		{
			cea2045IntermediateResponse *intermediateResponse = (cea2045IntermediateResponse *)message;

			m_ucm->processStartCyclingResponse(intermediateResponse);

			linkLayer->sendLinkLayerAck();

			break;
		}

		case IntermediateTypeCode::TERMINATE_CYCLING_RESPONSE:
		{
			cea2045IntermediateResponse *intermediateResponse = (cea2045IntermediateResponse *)message;

			m_ucm->processTerminateCyclingResponse(intermediateResponse);

			linkLayer->sendLinkLayerAck();

			break;
		}

		default:
			linkLayer->sendLinkLayerNak(LinkLayerNakCode::REQUEST_NOT_SUPPORTED);
			break;
	}
}

//======================================================================================

void ProcessMessageUCM::processDataLinkMessage(ILinkLayerCommSend *linkLayer, cea2045MessageHeader *message)
{
	if (!m_ucm->isMessageTypeSupported(MessageTypeCode::DATALINK))
	{
		linkLayer->sendLinkLayerNak(LinkLayerNakCode::REQUEST_NOT_SUPPORTED);
		return;
	}

	cea2045DataLink *dataLink = (cea2045DataLink *)message;

	DataLinkTypeCode messageType = ConvertEnums::convertDataLinkType(dataLink->opCode1);

	switch (messageType)
	{
	case DataLinkTypeCode::MAX_PAYLOAD_REQUEST:
		linkLayer->sendLinkLayerAck();

		cea2045DataLink response;

		response.msgType1 = DATALINK_MSG_TYP1;
		response.msgType2 = DATALINK_MSG_TYP2;
		response.length = htobe16(2);
		response.opCode1 = MAXPAYLOAD_RESP;
		response.opCode2 = (unsigned char)m_ucm->getMaxPayload();

		response.checksum = Checksum::calculate((unsigned char *)&response, sizeof(response) - 2);

		linkLayer->sendResponse((unsigned char*)&response, sizeof(response));

		break;

	case DataLinkTypeCode::MAX_PAYLOAD_RESPONSE:
		MaxPayloadLengthCode maxPayloadLength;

		maxPayloadLength = ConvertEnums::convertMaxPayloadLength(dataLink->opCode2);

		m_ucm->processMaxPayloadResponse(maxPayloadLength);

		linkLayer->sendLinkLayerAck();

		break;

	default:
		linkLayer->sendLinkLayerNak(LinkLayerNakCode::REQUEST_NOT_SUPPORTED);
	}

}

//======================================================================================

void ProcessMessageUCM::processInvalidMessage(ILinkLayerCommSend *linkLayer, cea2045MessageHeader *message)
{
	linkLayer->sendLinkLayerNak(LinkLayerNakCode::UNSUPPORTED_MESSAGE_TYPE);
}

//======================================================================================

void ProcessMessageUCM::processIncompleteMessage(ILinkLayerCommSend *linkLayer, const unsigned char *buffer, unsigned int numBytes)
{
	m_ucm->processIncompleteMessage(buffer, numBytes);

	linkLayer->sendLinkLayerNak(LinkLayerNakCode::MESSAGE_TIMEOUT);
}

} /* namespace cea2045 */
