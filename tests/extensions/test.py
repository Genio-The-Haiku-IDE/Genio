#!python3
# Copyright 2024, The Genio Team
# All rights reserved. Distributed under the terms of the MIT license.
# Author: Nexus6 <nexus6@disroot.org>

import time
from Be import BMessenger, BMessage, BEntry, BPath, entry_ref, \
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

    # Check if the reply indicates success (no error field means success)
    error = reply.GetInt32("error", 0)
    # Also check that it's not a B_MESSAGE_NOT_UNDERSTOOD reply
    is_error_reply = (reply.what == 0x42_4d_4e_55)  # 'BMNU' - B_MESSAGE_NOT_UNDERSTOOD

    if error == 0 and not is_error_reply:
        print(f"{file} opened in new Editor. Test *OpenEditor* PASSED!")
    else:
        print(f"Test *OpenEditor* FAILED! error: {error}, what: {hex(reply.what)}")
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
    print("test *Undo*: All changes undone.")


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


def CaretPosition():
    # Set selection to position the caret at a known location
    SetSelection(10, 0)

    message = BMessage(B_GET_PROPERTY)
    message.AddSpecifier("CaretPosition")
    message.AddSpecifier("SelectedEditor")
    reply = BMessage()
    genio.SendMessage(message, reply)

    caretInfo = BMessage()
    rez = reply.FindMessage("result", caretInfo)
    error = reply.GetInt32("error", 0)

    if rez == 0 and error == 0:
        line = caretInfo.GetInt32("line", -1)
        column = caretInfo.GetInt32("column", -1)
        offset = caretInfo.GetInt32("offset", -1)

        # Verify that we got valid values (all should be >= 0)
        if line > 0 and column > 0 and offset >= 0:
            print(f"test *CaretPosition* PASSED! Line: {line}, Column: {column}, Offset: {offset}")
        else:
            print(f"test *CaretPosition* FAILED! Invalid values - Line: {line}, Column: {column}, Offset: {offset}")
    else:
        print(f"test *CaretPosition* FAILED! Error: {error}")


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
    Ref()
    CaretPosition()


if __name__ == "__main__":
    main()