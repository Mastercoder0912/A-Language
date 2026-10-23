
#!/usr/bin/env python3
"""Track daily GitHub clone statistics without double-counting API windows."""

import json
import os
import sys
import urllib.error
import urllib.request
from datetime import datetime, timezone
from pathlib import Path

DATA_FILE = Path("data/clone-stats.json")


def main():
    token = os.environ.get("GH_TOKEN")
    repo = os.environ.get("GH_REPO", "Mastercoder0912/A-Language")

    if not token:
        sys.exit("Error: GH_TOKEN environment variable is missing.")

    url = f"https://api.github.com/repos/{repo}/traffic/clones?per=day"
    request = urllib.request.Request(
        url,
        headers={
            "Accept": "application/vnd.github+json",
            "Authorization": f"Bearer {token}",
            "X-GitHub-Api-Version": "2026-03-10",
        },
    )

    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            traffic = json.load(response)
    except urllib.error.HTTPError as exc:
        sys.exit(
            f"GitHub API error {exc.code}: {exc.read().decode(errors='replace')}"
        )
    except urllib.error.URLError as exc:
        sys.exit(f"Network error: {exc}")

    today = datetime.now(timezone.utc).date().isoformat()
    DATA_FILE.parent.mkdir(parents=True, exist_ok=True)

    if DATA_FILE.exists():
        with DATA_FILE.open(encoding="utf-8") as file:
            data = json.load(file)
    else:
        data = {
            "repository": repo,
            "tracked_since_utc": today,
            "baseline_note": (
                "Historical 126-clone report not included in verified total."
            ),
            "daily": {},
        }

    data.setdefault("daily", {})

    # Refresh dates returned by GitHub; preserve older saved dates.
    for entry in traffic.get("clones", []):
        day = entry["timestamp"][:10]
        data["daily"][day] = {
            "clones": entry["count"],
            "unique_cloners": entry["uniques"],
        }

    data["repository"] = repo
    data["last_updated_utc"] = datetime.now(
        timezone.utc
    ).isoformat()

    with DATA_FILE.open("w", encoding="utf-8") as file:
        json.dump(data, file, indent=2)
        file.write("\n")

    total = sum(
        entry["clones"] for entry in data["daily"].values()
    )

    print(f"Repository: {repo}")
    print(f"Recorded clone events: {total}")
    print(f"Days recorded: {len(data['daily'])}")
    print(f"Last updated (UTC): {data['last_updated_utc']}")


if __name__ == "__main__":
    main()
