from dataclasses import dataclass
from typing import Generator

import httpx
import parsel


@dataclass
class InfoItem:
    date: str
    sunrise_time: str | None
    sunset_time: str | None
    AQI: int | None

    def __post_init__(self):
        self.AQI = int(self.AQI) if self.AQI else None


class InfoParser:
    def __init__(self, html_str):
        self._selector = parsel.Selector(text=html_str)

    def parse(self) -> Generator[InfoItem, None, None]:
        table = self._selector.css("table")[1]
        ths = table.css("thead>tr>th")
        AQI_index, sr_ss_index = self._get_indexs(ths)
        trs = table.css("tbody>tr")
        yield from self._get_infos(AQI_index, sr_ss_index, trs)

    def _get_infos(
        self, AQI_index, sr_ss_index, trs
    ) -> Generator[InfoItem, None, None]:
        sunrise_time, sunset_time = None, None
        for tr in trs:
            td = tr.css("td")
            date = td[0].css("::text").get()
            times = td[sr_ss_index].css("::text").get()
            AQI = td[AQI_index].css("::text").get()
            if times:
                split_parts = times.split("/")
                sunrise_time = split_parts[0]
                sunset_time = split_parts[1]
            if not all([sunrise_time, sunset_time, AQI]):
                sunrise_time, sunset_time, AQI = None, None, None

            yield InfoItem(date, sunrise_time, sunset_time, AQI)

    def _get_indexs(self, ths) -> tuple[int, int]:
        AQI_index = None
        sr_ss_index = None
        for index, th in enumerate(ths):
            th: parsel.selector.Selector
            th_text = th.css("::text").get().strip()
            if th_text == "空气质量AQI":
                AQI_index = index
            elif th_text == "日出/日落":
                sr_ss_index = index
        if AQI_index is None or sr_ss_index is None:
            raise ValueError("未能找到所有必需的表头")

        return (AQI_index, sr_ss_index)


headers: dict[str, str] = {
    "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/146.0.0.0 Safari/537.36 Edg/146.0.0.0",
}


def grab_data(data_str: str):
    req = httpx.get(
        "https://datashareclub.com/weather/%E5%90%89%E6%9E%97/%E5%BB%B6%E8%BE%B9/101060308.html",
        headers=headers,
    )
    html: str = req.text
    info_parser = InfoParser(html)
    for info in info_parser.parse():
        if info.date == data_str:
            return (info.sunrise_time, info.sunset_time, info.AQI)


if __name__ == "__main__":
    a = grab_data("2026-04-01")
    print(a)
