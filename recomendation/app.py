import argparse
import json
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from model import ValidationError, recommend

STATIC_DIR = Path(__file__).with_name("static")


class RecommendationHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(STATIC_DIR), **kwargs)

    def guess_type(self, path):
        content_type = super().guess_type(path)
        if content_type.startswith("text/") or content_type == "application/javascript":
            return f"{content_type}; charset=utf-8"
        return content_type

    def log_message(self, format, *args):
        return

    def do_POST(self):
        if self.path != "/api/recommend":
            self.send_error(404)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            if length > 64_000:
                raise ValueError
            payload = json.loads(self.rfile.read(length))
            self._json_response(200, recommend(payload))
        except ValidationError as error:
            self._json_response(422, {"errors": error.errors})
        except (json.JSONDecodeError, TypeError, ValueError):
            self._json_response(400, {"error": "Некорректный запрос"})

    def _json_response(self, status, payload):
        body = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)


def run():
    parser = argparse.ArgumentParser(description="In-memory FS recommendation UI")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    args = parser.parse_args()

    address = f"http://{args.host}:{args.port}"
    server = ThreadingHTTPServer((args.host, args.port), RecommendationHandler)
    print(f"Рекомендатель запущен: {address}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nСервер остановлен")
    finally:
        server.server_close()


if __name__ == "__main__":
    run()