import csv
import json

with open(r"code.csv", encoding="utf8") as f:
    reader = csv.reader(f)
    data = [x for x in reader][0]

print(len(data))
henkan = dict(
    [(x, (i, "0x" + i.to_bytes(2, "little").hex().upper())) for i, x in enumerate(data)]
)

with open("code.json", "w", encoding="utf8") as f:
    json.dump(henkan, f, ensure_ascii=False, indent=2)
