import http.server
import socketserver

class SimpleHTTPRequestHandler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        # Get content length (data size) from the header
        content_length = int(self.headers['Content-Length'])
        # Read the data
        post_data = self.rfile.read(content_length)
        
        # Log the received data
        print(f"Received POST data: {post_data.decode('utf-8')}")
        
        # Respond with a 200 OK status and a message
        self.send_response(200)
        self.send_header('Content-Type', 'text/plain')
        self.end_headers()
        response = "POST request received"
        self.wfile.write(response.encode('utf-8'))

if __name__ == "__main__":
    PORT = 8080
    Handler = SimpleHTTPRequestHandler
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print(f"Serving on port {PORT}")
        httpd.serve_forever()
