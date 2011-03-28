import timing
from multiprocessing import Process, Queue
from main import window

import BaseHTTPServer

HOST_NAME = 'localhost'
PORT_NUMBER = 8888


class CAHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    
    def do_GET(s):
        s.send_response(200)
        s.send_header("Content-type", "text/html")
        s.end_headers()
        s.wfile.write("got it")
        print s
        print dir(s)
        print dir(s.server)


def go(q):
    q.put("Going")
    p2 = window(q)



if __name__ == "__main__":

    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class((HOST_NAME, PORT_NUMBER), CAHandler)

    q = Queue()
    p = Process(target=go, args=(q,))
    p.start()
    print q.get()

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    p.join()
    httpd.server_close()
    print "server stopped"

