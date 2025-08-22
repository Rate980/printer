import json

with open("code.json", "r") as file:
    data = json.load(file)


while True:
    inp = input(":")

    for ch in inp:
        code = data.get(ch)
        if code is not None:
            print(code)
        else:
            print(f"{ch} is not defined")
