#ifndef	__picos_rfmattr_h__
#define __picos_rfmattr_h__

// Attribute conversion for RF module drivers

#define	OBuffer			_dac (PicOSNode, OBuffer)
#define	RFInterface		_dac (PicOSNode, RFInterface)
#define	Receiving		_dac (PicOSNode, Receiving)
#define	TXOFF			_dac (PicOSNode, TXOFF)
#define	RXOFF			_dac (PicOSNode, RXOFF)
#define	Xmitting		_dac (PicOSNode, Xmitting)
#define	tx_event		_dac (PicOSNode, tx_event)
#define	statid			_dac (PicOSNode, statid)
#define	gbackoff()		(  ((PicOSNode*)TheStation)->_na_gbackoff ()  )
#define	rx_event		RFInterface

#endif
