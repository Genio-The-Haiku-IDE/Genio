#!python3
# Copyright 2024, The Genio Team
# All rights reserved. Distributed under the terms of the MIT license.
# Author: Nexus6 

import subprocess
from Be import BMessenger, BMessage, B_GET_PROPERTY

genio = BMessenger("application/x-vnd.Genio")


def GetSymbol():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Symbol")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    error = reply.GetInt32("error", 0)
    return result, error


def GetSelection():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Selection")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    error = reply.GetInt32("error", 0)
    return result, error


def OpenZeal(symbol):
    run = -1
    try:
        result = subprocess.run(["Zeal", symbol])
        run = result.returncode
    except Exception:
        print("Error executing Zeal.")
    if run != 0:
        print("Return code " + str(run))
        subprocess.run(["alert", "--warning",
                        "Could not run Zeal.\n"
                        "Ensure the package is installed!"])


def main():
    selection, error = GetSelection()
    if selection != "" and error == 0:
        OpenZeal(selection)
    else:
        symbol, error = GetSymbol()
        if symbol != "" and error == 0:
            OpenZeal(symbol)


if __name__ == "__main__":
    main()