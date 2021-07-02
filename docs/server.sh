#!/bin/sh

PORT=3000

cd "$(dirname "$0")/book"

[ -f index.html ] || {
	echo "No index.html file found."
    exit 1
}

echo "Go to: http://localhost:$PORT/"
python3 -m http.server "$PORT" >/dev/null 2>&1

