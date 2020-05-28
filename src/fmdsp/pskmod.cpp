// fskmod.cpp: implementation of the CPskMod class.
//
//  This class creates a PSK signal
//
// History:
//	2015-02-23  Initial creation MSW
//	2015-02-23
//////////////////////////////////////////////////////////////////////
//==========================================================================================
// + + +   This Software is released under the "Simplified BSD License"  + + +
//Copyright 2010 Moe Wheatley. All rights reserved.
//
//Redistribution and use in source and binary forms, with or without modification, are
//permitted provided that the following conditions are met:
//
//   1. Redistributions of source code must retain the above copyright notice, this list of
//	  conditions and the following disclaimer.
//
//   2. Redistributions in binary form must reproduce the above copyright notice, this list
//	  of conditions and the following disclaimer in the documentation and/or other materials
//	  provided with the distribution.
//
//THIS SOFTWARE IS PROVIDED BY Moe Wheatley ``AS IS'' AND ANY EXPRESS OR IMPLIED
//WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL Moe Wheatley OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
//ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//The views and conclusions contained in the software and documentation are those of the
//authors and should not be interpreted as representing official policies, either expressed
//or implied, of Moe Wheatley.
//=============================================================================
#include "pskmod.h"
#include "datatypes.h"
#include "psktables.h"

// local defines.................

#define BACK_SPACE_CODE 0x08	// define some control codes for
#define TXOFF_CODE 0x03			// control codes that can be placed in the input
#define TXON_CODE 0x02			// queue for various control functions
#define TXTOG_CODE 0x05
#define TX_CNTRL_AUTOSTOP 1
#define TX_CNTRL_ADDCWID 2
#define TX_CNTRL_NOSQTAIL 3


#define TX_CONSTANT 22000.0		// TX Amplitude Factor

#define PHZ_0 0			//index into SYMBOL_TBL specify various signal phase states
#define PHZ_90 1
#define PHZ_180 2
#define PHZ_270 3
#define PHZ_OFF 4

const TYPECPX SYMBOL_TBL[5] =
{
	{1.0,0.0},		//PHZ 0
	{0.0,1.0},		//PHZ 90
	{-1.0,0.0},		//PHZ 180
	{0.0,-1.0},		//PHZ 270
	{0.0,0.0}		//PHZ OFF
};

#define TX_OFF_STATE 0			//TX is off
#define TX_SENDING_STATE 1		//TX is sending text
#define TX_PAUSED_STATE 2		//TX is paused
#define TX_PREAMBLE_STATE 3		//TX sending starting preamble
#define TX_POSTAMBLE_STATE 4	//TX sending ending posteamble
#define TX_CWID_STATE 5			//TX sending CW ID
#define TX_TUNE_STATE 6			//TX is tuning mode




/////////////////////////////////////////////////////////////////////////////////
//	Construct PSK mod object
/////////////////////////////////////////////////////////////////////////////////
CPskMod::CPskMod()
{
	Init(12500.0, 31.25, BPSK_MODE);
}

/////////////////////////////////////////////////////////////////////////////////
//	Initialize variables with given SampleRate
/////////////////////////////////////////////////////////////////////////////////
void CPskMod::Init(TYPEREAL SampleRate, TYPEREAL SymbolRate, quint8 Mode)
{
int i;
	m_PSKmode = Mode;
	m_SampleRate = SampleRate;
	m_SymbPeriod= 1.0 / SymbolRate;
	m_SymbTimeAcc = 0.0;
	m_SymbTimeInc = 1.0 / m_SampleRate;
	m_LastPhase = PHZ_OFF;
	m_NextPhase = PHZ_OFF;
	m_ShapingAcc = 0.0;
	m_ShapingInc = K_2PI*SymbolRate/(2.0*m_SampleRate);

	m_AddEndingZero = false;
	m_Tail = 0;
	m_Head = 0;
	m_TXState = TX_SENDING_STATE;

	for(i=0; i<32; i++)		//create post/preamble tables
	{
		m_Preamble[i] = TXTOG_CODE;
		m_Postamble[i++] = TXON_CODE;
	}
	m_Preamble[i] = 0;		// null terminate these tables
	m_Postamble[i] = 0;
}

/////////////////////////////////////////////////////////////////////////////////
//	Create PSK Modulation Signal
/////////////////////////////////////////////////////////////////////////////////
void CPskMod::GenerateData(int Length, TYPEREAL SigAmplitude, TYPECPX* pOutData)
{
int i;
qint8 symbol = SYM_OFF;
TYPEREAL ShapeDown, ShapeUp;
	for(i=0; i<Length; i++)
	{
		m_SymbTimeAcc += m_SymbTimeInc;
		if(m_SymbTimeAcc >= m_SymbPeriod)
		{	//at next symbol time
			m_SymbTimeAcc -= m_SymbPeriod;
			m_ShapingAcc = 0.0;
			switch( m_PSKmode )				//get next symbol to send
			{
				case BPSK_MODE:
					symbol = GetNextBPSKSymbol();
					break;
				case QPSKU_MODE:
					symbol = GetNextQPSKSymbol();
					break;
				case QPSKL_MODE:
					symbol = GetNextQPSKSymbol();
					if(symbol==SYM_P90)		//rotate vectors the opposite way
						symbol = SYM_M90;
					else
						if(symbol==SYM_M90)
							symbol = SYM_P90;
					break;
				case TUNE_MODE:
					symbol = GetNextTuneSymbol();
					break;
			}
//symbol=SYM_NOCHANGE;
			m_LastPhase = m_NextPhase;
//symbol = SYM_P180;
			if(SYM_OFF == symbol)
			{
				m_NextPhase = PHZ_OFF;
			}
			else
			{
				m_NextPhase += symbol;
				m_NextPhase &= 0x03;
			}
		}
		ShapeDown = 0.5*cos(m_ShapingAcc) + 0.5;
		ShapeUp = 1.0 - ShapeDown;
		pOutData[i].re = ShapeDown * SYMBOL_TBL[m_LastPhase].re + ShapeUp * SYMBOL_TBL[m_NextPhase].re;
		pOutData[i].im = ShapeDown * SYMBOL_TBL[m_LastPhase].im + ShapeUp * SYMBOL_TBL[m_NextPhase].im;
		m_ShapingAcc += m_ShapingInc;
		pOutData[i].re *= SigAmplitude;
		pOutData[i].im *= SigAmplitude;
	}
}


/////////////////////////////////////////////////////////////
// called every symbol time to get next BPSK symbol and get the
// next character from the character Queue if no more symbols
// are left to send.
/////////////////////////////////////////////////////////////
quint8 CPskMod::GetNextBPSKSymbol(void)
{
quint8 symb;
quint8 ch;
	symb = m_Lastsymb;
	if( m_TxShiftReg == 0 )
	{
		if( m_AddEndingZero )		// if is end of code
		{
			symb = SYM_P180;		// end with a zero
			m_AddEndingZero = false;
		}
		else
		{
			ch = static_cast<quint8>(GetChar());			//get next character to xmit
			if( ch > TXTOG_CODE )	//if is not a control code
			{						//get next VARICODE codeword to send
				m_TxShiftReg = VARICODE_TABLE[ ch ];
				symb = SYM_P180;	//Start with a zero
			}
			else					// is a control code
			{
				switch( ch )
				{
				case TXON_CODE:
					symb = SYM_ON;
					break;
				case TXTOG_CODE:
					symb = SYM_P180;
					break;
				case TXOFF_CODE:
					symb = SYM_OFF;
					break;
				}
			}
		}
	}
	else			// is not end of code word so send next bit
	{
		if( m_TxShiftReg & 0x8000 )
			symb = SYM_NOCHANGE;
		else
			symb = SYM_P180;
		m_TxShiftReg = m_TxShiftReg<<1;	//point to next bit
		if( m_TxShiftReg == 0 )			// if at end of codeword
			m_AddEndingZero = true;		// need to send a zero nextime
	}
	m_Lastsymb = symb;
	return symb;
}


/////////////////////////////////////////////////////////////
// called every symbol time to get next QPSK symbol and get the
// next character from the character Queue if no more symbols
// are left to send.
/////////////////////////////////////////////////////////////
quint8 CPskMod::GetNextQPSKSymbol(void)
{
quint8 symb;
quint8 ch;
	symb = ConvolutionCodeTable[m_TxShiftReg&0x1F];	//get next convolution code
	m_TxShiftReg = m_TxShiftReg<<1;
	if( m_TxCodeWord == 0 )			//need to get next codeword
	{
		if( m_AddEndingZero )		//if need to add a zero
		{
			m_AddEndingZero = false;	//end with a zero
		}
		else
		{
			ch = static_cast<quint8>(GetChar());			//get next character to xmit
			if( ch > TXTOG_CODE )	//if is not a control code
			{						//get next VARICODE codeword to send
				m_TxCodeWord = VARICODE_TABLE[ ch ];
			}
			else					//is a control code
			{
				switch( ch )
				{
				case TXON_CODE:
					symb = SYM_ON;
					break;
				case TXTOG_CODE:
					m_TxCodeWord = 0;
					break;
				case TXOFF_CODE:
					symb = SYM_OFF;
					break;
				}
			}
		}
	}
	else
	{
		if(m_TxCodeWord&0x8000 )
		{
			m_TxShiftReg |= 1;
		}
		m_TxCodeWord = m_TxCodeWord<<1;
		if(m_TxCodeWord == 0)
			m_AddEndingZero = true;	//need to add another zero
	}
	return symb;
}

/////////////////////////////////////////////////////////////
// called every symbol time to get next Tune symbol
/////////////////////////////////////////////////////////////
quint8 CPskMod::GetNextTuneSymbol(void)
{
quint8 symb;
quint8 ch;
	ch = static_cast<quint8>(GetChar());			//get next character to xmit
	switch( ch )
	{
		case TXON_CODE:
			symb = SYM_ON;
			break;
		default:
			symb = SYM_OFF;
			break;
	}
	return symb;
}

/////////////////////////////////////////////////////////////
//get next character/symbol depending on TX state.
/////////////////////////////////////////////////////////////
qint16 CPskMod::GetChar()
{
quint8 ch = TXOFF_CODE;
static int inx=0;
static char TEST[] = "PSK Test 0123@$%^\b\b\b\b456789 abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ\r";
	switch( m_TXState )
	{
		case TX_OFF_STATE:		//is receiving
			ch = TXOFF_CODE;		//else turn off
			m_NeedShutoff = false;
			break;
		case TX_TUNE_STATE:
			ch = TXON_CODE;				// steady carrier
			if(	m_NeedShutoff)
			{
				if(	m_NeedCWid  )
				{
					m_TXState = TX_CWID_STATE;
					m_SavedMode = m_PSKmode;
					m_PSKmode = CW_MODE;
					m_NeedCWid = false;
					m_AmblePtr = 0;
					ch = TXOFF_CODE;
				}
				else
				{
					m_TXState = TX_OFF_STATE;
					m_AmblePtr = 0;
					ch = TXOFF_CODE;
					m_NeedShutoff = false;
				}
			}
			break;
		case TX_POSTAMBLE_STATE:		// ending sequence
			if( !(ch = m_Postamble[m_AmblePtr++] ) || m_NoSquelchTail)
			{
				m_NoSquelchTail = false;
				if(	m_NeedCWid  )
				{
					m_TXState = TX_CWID_STATE;
					m_SavedMode = m_PSKmode;
					m_PSKmode = CW_MODE;
					m_NeedCWid = false;
					m_AmblePtr = 0;
					ch = TXOFF_CODE;
				}
				else
				{
					m_TXState = TX_OFF_STATE;
					m_AmblePtr = 0;
					ch = TXOFF_CODE;
					m_NeedShutoff = false;
				}
			}
			break;
		case TX_PREAMBLE_STATE:			//starting sequence
			if( !(ch = m_Preamble[m_AmblePtr++] ))
			{
				m_TXState = TX_SENDING_STATE;
				m_AmblePtr = 0;
				ch = TXTOG_CODE;
			}
			break;
		case TX_SENDING_STATE:		//if sending text from TX window
			ch = static_cast<quint8>(GetTxChar());
//ch = 'e';
//ch = 150;
//ch = rand()&0x7F;
//ch = test++;
ch = TEST[inx++];
if((unsigned int)inx>= sizeof(TEST))
	inx = 0;
			if(	(ch == TXTOG_CODE) && m_NeedShutoff)
			{
				m_TXState = TX_POSTAMBLE_STATE;
			}
			else
			{
//				if( ch > 0 )
//					::PostMessage(m_hWnd, MSG_PSKCHARRDY,ch,-1);
			}
			m_AmblePtr = 0;
			break;

	}
	return( ch );
}


////////////////////////////////////////////////////////////////////////////
//		TX Queueing routines
/////////////////////////////////////////////////////////////////////////////
void CPskMod::PutTxQue(qint16 txchar)
{
	if( txchar <= TXTOG_CODE )	//is a tx control code
	{
		switch( txchar )
		{
			case TX_CNTRL_AUTOSTOP:
				m_TempNeedShutoff = true;
				if( m_TXState==TX_TUNE_STATE )
					m_NeedShutoff = true;
				break;
			case TX_CNTRL_ADDCWID:
				m_TempNeedCWid = true;
				break;
			case TX_CNTRL_NOSQTAIL:
				m_TempNoSquelchTail = true;
				break;
		}
	}
	else		//is a character to xmit
	{
		if( (txchar != BACK_SPACE_CODE) || (m_Head==m_Tail) )
		{
			m_pXmitQue[m_Head++] = static_cast<quint8>(txchar);
			if( m_Head >= TX_BUF_SIZE )
				m_Head = 0;
		}
		else	//see if is a backspace and if can delete it in the queue
		{
			if(--m_Head < 0 )		//look at last character in queue
				m_Head = 0;
			if( m_pXmitQue[m_Head] == BACK_SPACE_CODE)
			{								//if another backspace, leave it there
				if(++m_Head >= TX_BUF_SIZE )
					m_Head = 0;
				m_pXmitQue[m_Head++] = static_cast<quint8>(txchar);
				if( m_Head >= TX_BUF_SIZE )
					m_Head = 0;
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
qint16 CPskMod::GetTxChar()
{
int ch;
	if( m_Head != m_Tail )	//if something in Queue
	{
		ch = m_pXmitQue[m_Tail++] & 0x00FF;
		if( m_Tail >= TX_BUF_SIZE )
			m_Tail = 0;
	}
	else
		ch = TXTOG_CODE;		// if que is empty return TXTOG_CODE
	if(m_TempNeedShutoff)
	{
		m_TempNeedShutoff = false;
		m_NeedShutoff = true;
	}
	if(m_TempNoSquelchTail)
	{
		m_TempNoSquelchTail = false;
		m_NoSquelchTail = true;
	}
	return ch;
}

/////////////////////////////////////////////////////////////////////////////
void CPskMod::ClrQue()
{
	m_Tail = m_Head = 0;
	m_NoSquelchTail = false;
	m_TempNeedCWid = false;
	m_TempNeedShutoff = false;
	m_TempNoSquelchTail = false;
}

