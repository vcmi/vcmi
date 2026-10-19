"""Service configuration, /etc/vcmi-updates.conf."""

import configparser
import pathlib
from dataclasses import dataclass

DEFAULT_PATH = "/etc/vcmi-updates.conf"


@dataclass(frozen=True)
class Config:
    host: str
    port: int
    data_dir: pathlib.Path
    download_root: pathlib.Path
    download_url: str
    changelog_url: str  # full changelog shown to players, sent as 'changelogUrl'
    changelog_source: str  # ChangeLog.md that the release tool extracts sections from
    altstore_min_os_version: str  # 'minOSVersion' of AltStore entries of releases

    @property
    def releases_dir(self):
        return self.data_dir / "releases"

    @property
    def channels_dir(self):
        return self.data_dir / "channels"

    @property
    def altstore_template(self):
        return self.data_dir / "altstore.json"


def load_config(path=DEFAULT_PATH):
    parser = configparser.ConfigParser()
    with open(path, encoding="utf-8") as file:
        parser.read_file(file)
    host, _, port = parser.get("service", "listen").rpartition(":")
    return Config(
        host=host,
        port=int(port),
        data_dir=pathlib.Path(parser.get("service", "data_dir")),
        download_root=pathlib.Path(parser.get("downloads", "download_root")),
        download_url=parser.get("downloads", "download_url").rstrip("/"),
        changelog_url=parser.get("changelog", "changelog_url"),
        changelog_source=parser.get("changelog", "changelog_source"),
        altstore_min_os_version=parser.get("altstore", "min_os_version"),
    )
