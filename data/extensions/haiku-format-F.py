#!python3
# Copyright 2024, The Genio Team
# All rights reserved. Distributed under the terms of the MIT license.
# Author: Andrea Anzani <andrea.anzani@gmail.com>

import subprocess
from Be import BMessenger, BMessage, BPath, entry_ref, B_GET_PROPERTY

genio = BMessenger("application/x-vnd.Genio")


def Ref():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Ref")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    ref = entry_ref()
    rez = reply.FindRef("result", ref)
    if rez == 0:
        return BPath(ref).Path()
    else:
        return ""


def main():
    path = Ref()
    print("File path: " + path)
    if path != "":
        print("Current open editor:" + path)
        if path.endswith(".cpp") or path.endswith(".h"):
            print("File eligible for haiku-format")
            run = -1
            try:
                result = subprocess.run(["haiku-format", "-i", path])
                run = result.returncode
            except Exception:
                print("Error in executing haiku-format.")
            if run != 0:
                print("Return code " + str(run))
                subprocess.run(["alert", "--warning",
                                "Could not run haiku-format.\n"
                                "Ensure the package is installed!"])


if __name__ == "__main__":
    main()
