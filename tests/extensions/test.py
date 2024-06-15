#!python3
# Copyright 2024, The Genio Team
# All rights reserved. Distributed under the terms of the MIT license.
# Author: Nexus6 <nexus6@disroot.org>

import sys, time
from Be import BMessenger, BMessage, BString, BEntry, BPath, entry_ref, \
    B_EXECUTE_PROPERTY, B_GET_PROPERTY, B_SET_PROPERTY, B_CREATE_PROPERTY, B_COUNT_PROPERTIES

genio = BMessenger("application/x-vnd.Genio")


def OpenEditor():
    file = "./test_file"
    entry = BEntry(file)
    path = BPath()
    entry.GetPath(path)
    message = BMessage(B_CREATE_PROPERTY)
    message.AddSpecifier("Editor", path.Path())
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetInt32("result", -1)
    if result != -1:
        print(f"{file} opened in new Editor with Index {result}. Test *OpenEditor* PASSED!")
    else:
        print("Test *OpenEditor* FAILED!")
        exit(0)


def Selection():
    testString = "**EXAMPL"
    SetSelection(2, 8)
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Selection")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    # print("Expected result: " + testString)
    # print("Result: " + result)
    error = reply.GetInt32("error", 0)
    if result == testString and error == 0:
        print("test *Selection* PASSED!")
    else:
        print("test *Selection* FAILED! with error: " + str(error))


def SetSelection(index, length):
    message = BMessage(B_SET_PROPERTY)
    message.AddSpecifier("Selection", index, length)
    message.AddSpecifier("SelectedEditor")
    genio.SendMessage(message, None)


def GetLine(lineNumber):
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Line", lineNumber)
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    error = reply.GetInt32("error", 0)
    return result, error


def Line():
    testString = "\t\tThis is a new line\n"
    message = BMessage(B_SET_PROPERTY)
    message.AddSpecifier("Line", 2)
    message.AddSpecifier("SelectedEditor")
    message.AddString("data", testString)
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    error = reply.GetInt32("error", 0)

    result, error = GetLine(2)
    # print("Expected result: " + testString)
    # print("Result: " + result)
    if result == testString and error == 0:
        print("test *Line* PASSED!")
    else:
        print("test *Line* FAILED! with error: " + str(error))


def CountLines():
    message = BMessage(B_COUNT_PROPERTIES)
    message.AddSpecifier("Line")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetInt32("result", -1)
    return result


def Insert():
    testString = "**NEW TEXT**"

    message = BMessage(B_SET_PROPERTY)
    message.AddSpecifier("Text", 12)
    message.AddSpecifier("SelectedEditor")
    message.AddString("data",testString)
    genio.SendMessage(message, None)

    SetSelection(12, len(testString))

    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Selection")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    result = reply.GetString("result", "")
    # print("Expected result: " + testString)
    # print("Result: " + result)
    error = reply.GetInt32("error", 0)
    if result == testString and error == 0:
        print("test *Insert* PASSED!")
    else:
        print("test *Insert* FAILED! with error: " + str(error))


def Append():
    testString = "**NEW LINE**"
    newLine = "\n" + testString

    message = BMessage(B_SET_PROPERTY)
    message.AddSpecifier("Text")
    message.AddSpecifier("SelectedEditor")
    message.AddString("data", newLine)
    genio.SendMessage(message, None)

    lineCount = CountLines()
    result, error = GetLine(lineCount-1)
    # print("Expected result: " + testString)
    # print("Result: " + result)
    if result == testString and error == 0:
        print("test *Append* PASSED!")
    else:
        print("test *Append* FAILED! with error: " + str(error))


def Undo():
    message = BMessage(B_EXECUTE_PROPERTY)
    message.AddSpecifier("Undo")
    message.AddSpecifier("SelectedEditor")
    genio.SendMessage(message, None)


def Ref():
    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("Ref")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)
    ref = entry_ref()
    rez = reply.FindRef("result", ref)
    if rez == 0:
        fullpath = BPath(ref).Path()
        print("test *Ref* PASSED! path: " + fullpath)
    else:
        print("test *Ref* FAILED!")


def main():
    OpenEditor()
    time.sleep(1)
    Selection()
    Line()
    Undo()
    Insert()
    Undo()
    Append()
    Undo()
    print("All changes undone.")
    Ref()


if __name__ == "__main__":
    main()