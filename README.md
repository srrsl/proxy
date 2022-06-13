# proxy
<br />
- It will listen on two port of clients on 6010 and 6020 for example and then redirect them to port 6011 of localhost. you can add more port for clients 		using following command:<br />
			```> ./dispatcher_server <local host ip> <local port1> <local port2> ... <local portN> <forward host ip> <forward port><br />```
<br />
- In command line first build proxy file and then run it using following commands on your localhost:<br />
			```> make<br />```
			```> ./dispatcher_server 127.0.0.1 6010 6020 127.0.0.1 6011<br />```
<br />
- Run a very simple server and client in python to test proxy server.<br />
	first run server by the following command :<br />
			```> python server<br />```
	It will run a server on port "6011"<br />
	and then <br />
			```> python client<br />```
	It will send data on port 6010 which you can change its port in client.py.<br />
