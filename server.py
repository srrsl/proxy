import socket
import threading


class ClientThread(threading.Thread):
    def __init__(self, clientAddress, clientsocket):
        threading.Thread.__init__(self)
        self.csocket = clientsocket
        self.received_buffer = b''
        print ("New connection added: ", clientAddress)

    def buffer_handler(self):
        while True:
            Len = int(self.received_buffer[:4])
            print(Len)

            current_packet = self.received_buffer[4:(Len + 4)]
            print(current_packet)

            self.received_buffer = self.received_buffer[(Len + 4):]

            print("from client", self.received_buffer)
            self.csocket.send(current_packet)

            if Len != len(current_packet) or self.received_buffer==b'' :
                print(len(current_packet), "-", Len)
                break

        print("end of buffer handler process ...")

    def run(self):
        print ("Connection from : ", clientAddress)
        while True:
            # self.received_buffer += self.csocket.recv(4096)
            self.received_buffer = self.csocket.recv(4096)

            # second_thread = threading.Thread(target=self.buffer_handler)
            # second_thread.start()

            self.csocket.send(self.received_buffer)

        print ("Client at ", clientAddress , " disconnected...")



LOCALHOST = "127.0.0.1"
PORT = 6011
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server.bind((LOCALHOST, PORT))
print("Server started")
print("Waiting for client request..")

while True:
    server.listen(1)
    clientsock, clientAddress = server.accept()
    newthread = ClientThread(clientAddress, clientsock)
    newthread.start()