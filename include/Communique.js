var CommuniqueClient = function( websocketUri )
{
	this.nullUserReference='\x00\x00\x00\x00'; // Used as the user reference for info messages
	this.REQUEST=0;
	this.RESPONSE=1;
	this.INFO=2;
	this.REQUESTERROR=3;
	this.REQUEST_STRING='\x00';
	this.RESPONSE_STRING='\x01';
	this.INFO_STRING='\x02';
	this.REQUESTERROR_STRING='\x03';
	
	this.websocket = new WebSocket(websocketUri);
	this.websocket.binaryType = 'arraybuffer';
	this.websocket.onopen = this.handleOpen.bind(this);
	this.websocket.onclose = this.handleClose.bind(this);
	this.websocket.onmessage = this.handleMessage.bind(this);
	this.websocket.onerror = this.handleError.bind(this);
	
	// Create default handlers that don't do anything. User should
	// override these.
	this.onopen = function(evt) {};
	this.oninfo = function(message) {};
	this.onrequest = function(request) {};
	this.onclose = function(evt) {};
	
	this.responseHandlers=[];
};

CommuniqueClient.prototype.close = function()
{
	this.websocket.close();
}

CommuniqueClient.prototype.sendInfo = function( message )
{
	this.websocket.send(this.INFO_STRING+this.nullUserReference+message);
}

CommuniqueClient.prototype.sendRequest = function( message, newHandler )
{
	// Need to find a user reference that is free
	newUserReference=124;
	referenceIsUsed=true;
	while( referenceIsUsed )
	{
		referenceIsUsed=false;
		for( index=0; index<this.responseHandlers.length; index++ )
		{
			if( this.responseHandlers[index].userReference==newUserReference )
			{
				referenceIsUsed=true;
				newUserReference++;
				break;
			}
		}
	}
	
	// Now have a spare user reference, I can store the handler along with the reference
	this.responseHandlers.push( {userReference: newUserReference, handler: newHandler } )

	myBuffer=new ArrayBuffer(message.length+5);
	dataView=new DataView(myBuffer);
	dataView.setUint8( 0, this.REQUEST );
	dataView.setUint8( 0, this.REQUEST );
	dataView.setUint32( 1, newUserReference );
	
	for( index=0; index<message.length; index++ )
	{
		dataView.setUint8( index+5, message.charCodeAt(index) );
	}
	
	this.websocket.send(myBuffer);
}


CommuniqueClient.prototype.handleOpen = function( event )
{
	this.onopen( event );
}

CommuniqueClient.prototype.handleClose = function( event )
{
	this.onclose( event );
}

CommuniqueClient.prototype.handleMessage = function( event )
{
	dataView=new DataView(event.data);
	var messageType=dataView.getUint8(0);
	var userReference=dataView.getUint32(1);
	var message=String.fromCharCode.apply(null, new Uint8Array(event.data,5) );

//	var messageType=(new Uint8Array(event.data,0,1))[0];
//	var userReference=(new Uint32Array(event.data.slice(1,5)))[0];
//	var message=String.fromCharCode.apply(null, new Uint8Array(event.data,5) );

	if( messageType==this.INFO ) this.oninfo( message );
	else if( messageType==this.REQUEST ) this.onrequest( message );
	else if( messageType==this.RESPONSE )
	{
		// Try and find the handler for this response
		for( index=0; index<this.responseHandlers.length; index++ )
		{
			if( this.responseHandlers[index].userReference==userReference ) break;
		}
	
		if( index!=this.responseHandlers.length )
		{
			handler=this.responseHandlers[index].handler;
			this.responseHandlers.splice(index,1);
			handler( message );
		}
		else alert("Invalid response received");
	}
}

CommuniqueClient.prototype.handleError = function( event )
{
	alert( "websocket error: "+event );
}
