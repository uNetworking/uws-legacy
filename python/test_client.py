import uWebSockets

import threading
import time

class TestClient(uWebSockets.WebSocketClient):
    def on_message(self, message):
        print("Got message: %s" % repr(message))
        time.sleep(1)
        self.close()
        
    def test(self):
        print("What")
    
    def on_open(self):
        print("Opened!")
        self.send("Hello, World!")
        
    def on_close(self, code, message):
        print("Closed %d %s" % (code, message))
        
    def close(self):
        print("Overrode close here")
        super(TestClient, self).close()
        

ws = TestClient("ws://localhost:3000")
ws.run(True)
time.sleep(5)
ws.close()
