Communique
==========

A simple websocket based C++ library for remote procedure calls. Based on WebSocket++.

On top of websocket adds remote procedure call like functionality, i.e. you can send a message and get the reply filtered out from any other message traffic that happens to be going on. The reply is passed to a callback function supplied when the message is sent.

Currently very much in beta. It works under perfect conditions but error handling is pretty poor at the moment.
