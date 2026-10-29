"""HTTP service for GET /autoupdate.json and /altstore.json."""

import argparse
import json
import logging
import sys
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

from .config import DEFAULT_PATH, load_config
from .decide import decide, parse_request
from .errors import DataError
from .store import data_signature, load_snapshot

RELOAD_INTERVAL = 10
CACHE_CONTROL = "public, max-age=300"

log = logging.getLogger("vcmi-updates")


class Service:
    """Holds the current snapshot."""

    def __init__(self, config):
        self.config = config
        self.signature = data_signature(config)
        self.snapshot = load_snapshot(config, log.warning)

    def reload_if_changed(self):
        signature = data_signature(self.config)
        if signature == self.signature:
            return
        self.signature = signature
        try:
            self.snapshot = load_snapshot(self.config, log.warning)
            log.info("data reloaded")
        except DataError as error:
            log.error("data not reloaded, keeping previous data: %s", error)

    def respond(self, path, params):
        """Returns the response for a request as a JSON-serializable dict, or None if there is nothing at path."""
        if path == "/autoupdate.json":
            return decide(parse_request(params), self.snapshot)
        if path == "/altstore.json":
            return self.snapshot.altstore
        return None


class Handler(BaseHTTPRequestHandler):
    service = None  # set by main()

    def do_GET(self):
        url = urllib.parse.urlsplit(self.path)
        params = {name: values[0] for name, values in urllib.parse.parse_qs(url.query).items()}
        try:
            response = self.service.respond(url.path, params)
            body = None if response is None else json.dumps(response).encode("utf-8")
        except Exception:
            log.exception("failed to handle request %s", self.path)
            self.send_error(500)
            return
        if body is None:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", CACHE_CONTROL)
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        # Default implementation logs client addresses
        pass


def _reload_loop(service):
    while True:
        time.sleep(RELOAD_INTERVAL)
        try:
            service.reload_if_changed()
        except Exception:
            log.exception("reload failed")


def main(argv=None):
    parser = argparse.ArgumentParser(description="VCMI update service")
    parser.add_argument("--config", default=DEFAULT_PATH)
    args = parser.parse_args(argv)
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(message)s")

    config = load_config(args.config)
    try:
        Handler.service = Service(config)
    except DataError as error:
        log.error("invalid data: %s", error)
        return 1

    threading.Thread(target=_reload_loop, args=(Handler.service,), daemon=True).start()
    server = ThreadingHTTPServer((config.host, config.port), Handler)
    log.info("listening on %s:%d", config.host, config.port)
    server.serve_forever()
    return 0


if __name__ == "__main__":
    sys.exit(main())
