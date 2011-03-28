import timing
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


def go(q):
    q.put("ASDF")
    p2 = window()


from multiprocessing import Process, Queue

if __name__ == "__main__":
    #p2 = window()
    q = Queue()
    p = Process(target=go, args=(q,))
    p.start()
    print q.get()
    print "WOOT"


"""

if __name__ == "__main__":

    server_class = BaseHTTPServer.HTTPServer
    httpd = server_class((HOST_NAME, PORT_NUMBER), CAHandler)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print "server stopped"

"""
