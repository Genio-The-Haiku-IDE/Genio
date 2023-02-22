//
// Created by Alex on 2020/1/28. (https://github.com/microsoft/language-server-protocol)
// Additional changes by Andrea Anzani <andrea.anzani@gmail.com>
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// Most of the code comes from clangd(Protocol.h)

#ifndef LSP_PROTOCOL_H
#define LSP_PROTOCOL_H
#include <string>
#include <vector>
#include <tuple>
#include <map>
#include <memory>
#include "uri.h"
#define MAP_JSON(...) {j = {__VA_ARGS__};}
#define MAP_KEY(KEY) {#KEY, value.KEY}
#define MAP_TO(KEY, TO) {KEY, value.TO}
#define MAP_KV(K, ...) {K, {__VA_ARGS__}}
#define FROM_KEY(KEY) if (j.contains(#KEY)) j.at(#KEY).get_to(value.KEY);
#define JSON_SERIALIZE(Type, TO, FROM) \
    namespace nlohmann { \
        template <> struct adl_serializer<Type> { \
            static void to_json(json& j, const Type& value) TO \
            static void from_json(const json& j, Type& value) FROM \
        }; \
    }
using TextType = string_ref;
enum class ErrorCode {
    // Defined by JSON RPC.
    ParseError = -32700,
    InvalidRequest = -32600,
    MethodNotFound = -32601,
    InvalidParams = -32602,
    InternalError = -32603,
    ServerNotInitialized = -32002,
    UnknownErrorCode = -32001,
    // Defined by the protocol.
    RequestCancelled = -32800,
};
class LSPError {
public:
    std::string Message;
    ErrorCode Code;
    static char ID;
    LSPError(std::string Message, ErrorCode Code)
            : Message(std::move(Message)), Code(Code) {}
};

struct TextDocumentIdentifier {
    /// The text document's URI.
    DocumentUri uri;
};
JSON_SERIALIZE(TextDocumentIdentifier, MAP_JSON(MAP_KEY(uri)), {});

struct VersionedTextDocumentIdentifier : public TextDocumentIdentifier {
    int version = 0;
};
JSON_SERIALIZE(VersionedTextDocumentIdentifier, MAP_JSON(MAP_KEY(uri), MAP_KEY(version)), {});

struct Position {
    /// Line position in a document (zero-based).
    int line = 0;
    /// Character offset on a line in a document (zero-based).
    /// WARNING: this is in UTF-16 codepoints, not bytes or characters!
    /// Use the functions in SourceCode.h to construct/interpret Positions.
    int character = 0;
    friend bool operator==(const Position &LHS, const Position &RHS) {
        return std::tie(LHS.line, LHS.character) ==
               std::tie(RHS.line, RHS.character);
    }
    friend bool operator!=(const Position &LHS, const Position &RHS) {
        return !(LHS == RHS);
    }
    friend bool operator<(const Position &LHS, const Position &RHS) {
        return std::tie(LHS.line, LHS.character) <
               std::tie(RHS.line, RHS.character);
    }
    friend bool operator<=(const Position &LHS, const Position &RHS) {
        return std::tie(LHS.line, LHS.character) <=
               std::tie(RHS.line, RHS.character);
    }
};
JSON_SERIALIZE(Position, MAP_JSON(MAP_KEY(line), MAP_KEY(character)), {FROM_KEY(line);FROM_KEY(character)});

struct Range {
    /// The range's start position.
    Position start;

    /// The range's end position.
    Position end;

    friend bool operator==(const Range &LHS, const Range &RHS) {
        return std::tie(LHS.start, LHS.end) == std::tie(RHS.start, RHS.end);
    }
    friend bool operator!=(const Range &LHS, const Range &RHS) {
        return !(LHS == RHS);
    }
    friend bool operator<(const Range &LHS, const Range &RHS) {
        return std::tie(LHS.start, LHS.end) < std::tie(RHS.start, RHS.end);
    }
    bool contains(Position Pos) const { return start <= Pos && Pos < end; }
    bool contains(Range Rng) const {
        return start <= Rng.start && Rng.end <= end;
    }
};
JSON_SERIALIZE(Range, MAP_JSON(MAP_KEY(start), MAP_KEY(end)), {FROM_KEY(start);FROM_KEY(end)});

struct Location {
    /// The text document's URI.
    std::string uri;
    Range range;

    friend bool operator==(const Location &LHS, const Location &RHS) {
        return LHS.uri == RHS.uri && LHS.range == RHS.range;
    }
    friend bool operator!=(const Location &LHS, const Location &RHS) {
        return !(LHS == RHS);
    }
    friend bool operator<(const Location &LHS, const Location &RHS) {
        return std::tie(LHS.uri, LHS.range) < std::tie(RHS.uri, RHS.range);
    }
};
JSON_SERIALIZE(Location, MAP_JSON(MAP_KEY(uri), MAP_KEY(range)), {FROM_KEY(uri);FROM_KEY(range)});

struct TextEdit {
    /// The range of the text document to be manipulated. To insert
    /// text into a document create a range where start === end.
    Range range;

    /// The string to be inserted. For delete operations use an
    /// empty string.
    std::string newText;
};
JSON_SERIALIZE(TextEdit, MAP_JSON(MAP_KEY(range), MAP_KEY(newText)), {FROM_KEY(range);FROM_KEY(newText);});

struct TextDocumentItem {
    /// The text document's URI.
    DocumentUri uri;

    /// The text document's language identifier.
    string_ref languageId;

    /// The version number of this document (it will strictly increase after each
    int version = 0;

    /// The content of the opened text document.
    string_ref text;
};
JSON_SERIALIZE(TextDocumentItem, MAP_JSON(
                MAP_KEY(uri), MAP_KEY(languageId), MAP_KEY(version), MAP_KEY(text)), {});

enum class TraceLevel {
    Off = 0,
    Messages = 1,
    Verbose = 2,
};
enum class TextDocumentSyncKind {
    /// Documents should not be synced at all.
    None = 0,
    /// Documents are synced by always sending the full content of the document.
    Full = 1,
    /// Documents are synced by sending the full content on open.  After that
    /// only incremental updates to the document are send.
    Incremental = 2,
};
enum class CompletionItemKind {
    Missing = 0,
    Text = 1,
    Method = 2,
    Function = 3,
    Constructor = 4,
    Field = 5,
    Variable = 6,
    Class = 7,
    Interface = 8,
    Module = 9,
    Property = 10,
    Unit = 11,
    Value = 12,
    Enum = 13,
    Keyword = 14,
    Snippet = 15,
    Color = 16,
    File = 17,
    Reference = 18,
    Folder = 19,
    EnumMember = 20,
    Constant = 21,
    Struct = 22,
    Event = 23,
    Operator = 24,
    TypeParameter = 25,
};
enum class SymbolKind {
    File = 1,
    Module = 2,
    Namespace = 3,
    Package = 4,
    Class = 5,
    Method = 6,
    Property = 7,
    Field = 8,
    Constructor = 9,
    Enum = 10,
    Interface = 11,
    Function = 12,
    Variable = 13,
    Constant = 14,
    String = 15,
    Number = 16,
    Boolean = 17,
    Array = 18,
    Object = 19,
    Key = 20,
    Null = 21,
    EnumMember = 22,
    Struct = 23,
    Event = 24,
    Operator = 25,
    TypeParameter = 26
};
enum class OffsetEncoding {
    // Any string is legal on the wire. Unrecognized encodings parse as this.
    UnsupportedEncoding,
    // Length counts code units of UTF-16 encoded text. (Standard LSP behavior).
    UTF16,
    // Length counts bytes of UTF-8 encoded text. (Clangd extension).
    UTF8,
    // Length counts codepoints in unicode text. (Clangd extension).
    UTF32,
};
enum class MarkupKind {
    PlainText,
    Markdown,
};
enum class ResourceOperationKind {
    Create,
    Rename,
    Delete
};
enum class FailureHandlingKind {
    Abort,
    Transactional,
    Undo,
    TextOnlyTransactional
};
NLOHMANN_JSON_SERIALIZE_ENUM(OffsetEncoding, {
    {OffsetEncoding::UnsupportedEncoding, "unspported"},
    {OffsetEncoding::UTF8, "utf-8"},
    {OffsetEncoding::UTF16, "utf-16"},
    {OffsetEncoding::UTF32, "utf-32"},
})
NLOHMANN_JSON_SERIALIZE_ENUM(MarkupKind, {
    {MarkupKind::PlainText, "plaintext"},
    {MarkupKind::Markdown, "markdown"},
})
NLOHMANN_JSON_SERIALIZE_ENUM(ResourceOperationKind, {
    {ResourceOperationKind::Create, "create"},
    {ResourceOperationKind::Rename, "rename"},
    {ResourceOperationKind::Delete, "dename"}
})
NLOHMANN_JSON_SERIALIZE_ENUM(FailureHandlingKind, {
    {FailureHandlingKind::Abort, "abort"},
    {FailureHandlingKind::Transactional, "transactional"},
    {FailureHandlingKind::Undo, "undo"},
    {FailureHandlingKind::TextOnlyTransactional, "textOnlyTransactional"}
})

struct ClientCapabilities {
    /// The supported set of SymbolKinds for workspace/symbol.
    /// workspace.symbol.symbolKind.valueSet
    std::vector<SymbolKind> WorkspaceSymbolKinds;
    /// Whether the client accepts diagnostics with codeActions attached inline.
    /// textDocument.publishDiagnostics.codeActionsInline.
    bool DiagnosticFixes = true;

    /// Whether the client accepts diagnostics with related locations.
    /// textDocument.publishDiagnostics.relatedInformation.
    bool DiagnosticRelatedInformation = true;

    /// Whether the client accepts diagnostics with category attached to it
    /// using the "category" extension.
    /// textDocument.publishDiagnostics.categorySupport
    bool DiagnosticCategory = true;

    /// Client supports snippets as insert text.
    /// textDocument.completion.completionItem.snippetSupport
    bool CompletionSnippets = false;

    bool CompletionDeprecated = true;

    /// Client supports completions with additionalTextEdit near the cursor.
    /// This is a clangd extension. (LSP says this is for unrelated text only).
    /// textDocument.completion.editsNearCursor
    bool CompletionFixes = true;

    /// Client supports hierarchical document symbols.
    bool HierarchicalDocumentSymbol = true;

    /// Client supports processing label offsets instead of a simple label string.
    bool OffsetsInSignatureHelp = true;

    /// The supported set of CompletionItemKinds for textDocument/completion.
    /// textDocument.completion.completionItemKind.valueSet
    std::vector<CompletionItemKind> CompletionItemKinds;

    /// Client supports CodeAction return value for textDocument/codeAction.
    /// textDocument.codeAction.codeActionLiteralSupport.
    bool CodeActionStructure = true;
    /// Supported encodings for LSP character offsets. (clangd extension).
    std::vector<OffsetEncoding> offsetEncoding = {OffsetEncoding::UTF8};
    /// The content format that should be used for Hover requests.
    std::vector<MarkupKind> HoverContentFormat = {MarkupKind::PlainText};

    bool ApplyEdit = false;
    bool DocumentChanges = false;
    ClientCapabilities() {
        for (int i = 1; i <= 26; ++i) {
            WorkspaceSymbolKinds.push_back((SymbolKind) i);
        }
        for (int i = 0; i <= 25; ++i) {
            CompletionItemKinds.push_back((CompletionItemKind) i);
        }
    }
};
JSON_SERIALIZE(ClientCapabilities,MAP_JSON(
            MAP_KV("textDocument",
                MAP_KV("publishDiagnostics", // PublishDiagnosticsClientCapabilities
                        MAP_TO("categorySupport", DiagnosticCategory),
                        MAP_TO("codeActionsInline", DiagnosticFixes),
                        MAP_TO("relatedInformation", DiagnosticRelatedInformation),
                ),
                MAP_KV("completion", // CompletionClientCapabilities
                        MAP_KV("completionItem",
                                MAP_TO("snippetSupport", CompletionSnippets),
                                MAP_TO("deprecatedSupport", CompletionDeprecated)),
                        MAP_KV("completionItemKind", MAP_TO("valueSet", CompletionItemKinds)),
                        MAP_TO("editsNearCursor", CompletionFixes)
                ),
                MAP_KV("codeAction", MAP_TO("codeActionLiteralSupport", CodeActionStructure)),
                MAP_KV("documentSymbol", MAP_TO("hierarchicalDocumentSymbolSupport", HierarchicalDocumentSymbol)),
                MAP_KV("hover",  //HoverClientCapabilities
                        MAP_TO("contentFormat", HoverContentFormat)),
                MAP_KV("signatureHelp", MAP_KV("signatureInformation", MAP_KV("parameterInformation", MAP_TO("labelOffsetSupport", OffsetsInSignatureHelp))))),
            MAP_KV("workspace", // WorkspaceEditClientCapabilities
                    MAP_KV("symbol", // WorkspaceSymbolClientCapabilities
                            MAP_KV("symbolKind",
                                    MAP_TO("valueSet", WorkspaceSymbolKinds))),
                    MAP_TO("applyEdit", ApplyEdit),
                    MAP_KV("workspaceEdit", // WorkspaceEditClientCapabilities
                            MAP_TO("documentChanges", DocumentChanges))),
            MAP_TO("offsetEncoding", offsetEncoding)), {});

struct ServerCapabilities {
    json capabilities;
    /**
     * Defines how text documents are synced. Is either a detailed structure defining each notification or
     * for backwards compatibility the TextDocumentSyncKind number. If omitted it defaults to `TextDocumentSyncKind.None`.
     */
    TextDocumentSyncKind textDocumentSync = TextDocumentSyncKind::None;
    bool resolveProvider = false;
    std::vector<std::string> executeCommands;
    std::vector<std::string> signatureHelpTrigger;
    std::vector<std::string> formattingTrigger;
    std::vector<std::string> completionTrigger;
    bool hasProvider(std::string &name) {
        if (capabilities.contains(name)) {
            if (capabilities[name].type() == json::value_t::boolean) {
                return capabilities["name"];
            }
        }
        return false;
    }
};
JSON_SERIALIZE(ServerCapabilities, {}, {
    value.capabilities = j;
    FROM_KEY(textDocumentSync);
    j["documentOnTypeFormattingProvider"]["firstTriggerCharacter"].get_to(value.formattingTrigger);
    j["completionProvider"]["resolveProvider"].get_to(value.resolveProvider);
    j["completionProvider"]["triggerCharacters"].get_to(value.completionTrigger);
    j["executeCommandProvider"]["commands"].get_to(value.executeCommands);
});

struct ClangdCompileCommand {
    TextType workingDirectory;
    std::vector<TextType> compilationCommand;
};
JSON_SERIALIZE(ClangdCompileCommand,MAP_JSON(
        MAP_KEY(workingDirectory), MAP_KEY(compilationCommand)), {});

struct ConfigurationSettings {
    // Changes to the in-memory compilation database.
    // The key of the map is a file name.
    std::map<std::string, ClangdCompileCommand> compilationDatabaseChanges;
};
JSON_SERIALIZE(ConfigurationSettings, MAP_JSON(MAP_KEY(compilationDatabaseChanges)), {});

struct InitializationOptions {
    // What we can change throught the didChangeConfiguration request, we can
    // also set through the initialize request (initializationOptions field).
    ConfigurationSettings configSettings;

    option<TextType> compilationDatabasePath;
    // Additional flags to be included in the "fallback command" used when
    // the compilation database doesn't describe an opened file.
    // The command used will be approximately `clang $FILE $fallbackFlags`.
    std::vector<TextType> fallbackFlags;

    /// Clients supports show file status for textDocument/clangd.fileStatus.
    bool clangdFileStatus = false;
};
JSON_SERIALIZE(InitializationOptions, MAP_JSON(
                MAP_KEY(configSettings),
                MAP_KEY(compilationDatabasePath),
                MAP_KEY(fallbackFlags),
                MAP_KEY(clangdFileStatus)), {});

struct InitializeParams {
    unsigned processId = 0;
    ClientCapabilities capabilities;
    option<DocumentUri> rootUri;
    option<TextType> rootPath;
    InitializationOptions initializationOptions;
};
JSON_SERIALIZE(InitializeParams, MAP_JSON(
        MAP_KEY(processId),
        MAP_KEY(capabilities),
        MAP_KEY(rootUri),
        MAP_KEY(initializationOptions),
        MAP_KEY(rootPath)), {});

enum class MessageType {
    /// An error message.
            Error = 1,
    /// A warning message.
            Warning = 2,
    /// An information message.
            Info = 3,
    /// A log message.
            Log = 4,
};
struct ShowMessageParams {
    /// The message type.
    MessageType type = MessageType::Info;
    /// The actual message.
    std::string message;
};
JSON_SERIALIZE(ShowMessageParams, {}, {FROM_KEY(type); FROM_KEY(message)});

struct Registration {
    /**
     * The id used to register the request. The id can be used to deregister
     * the request again.
     */
    TextType id;
    /**
     * The method / capability to register for.
     */
    TextType method;
};
JSON_SERIALIZE(Registration, MAP_JSON(MAP_KEY(id), MAP_KEY(method)), {});

struct RegistrationParams {
    std::vector<Registration> registrations;
};
JSON_SERIALIZE(RegistrationParams, MAP_JSON(MAP_KEY(registrations)), {});

struct UnregistrationParams {
    std::vector<Registration> unregisterations;
};
JSON_SERIALIZE(UnregistrationParams, MAP_JSON(MAP_KEY(unregisterations)), {});

struct DidOpenTextDocumentParams {
/// The document that was opened.
    TextDocumentItem textDocument;
};
JSON_SERIALIZE(DidOpenTextDocumentParams, MAP_JSON(MAP_KEY(textDocument)), {});

struct DidCloseTextDocumentParams {
    /// The document that was closed.
    TextDocumentIdentifier textDocument;
};
JSON_SERIALIZE(DidCloseTextDocumentParams, MAP_JSON(MAP_KEY(textDocument)), {});

struct DidSaveTextDocumentParams {
  /// The document that was saved.
  TextDocumentIdentifier textDocument;
};
JSON_SERIALIZE(DidSaveTextDocumentParams, MAP_JSON(MAP_KEY(textDocument)), {});

struct TextDocumentContentChangeEvent {
    /// The range of the document that changed.
    option<Range> range;

    /// The length of the range that got replaced.
    //xed option<int> rangeLength;
    /// The new text of the range/document.
    std::string text;
};
JSON_SERIALIZE(TextDocumentContentChangeEvent, MAP_JSON(MAP_KEY(range)/*, MAP_KEY(rangeLength)*/, MAP_KEY(text)), {});

struct DidChangeTextDocumentParams {
    /// The document that did change. The version number points
    /// to the version after all provided content changes have
    /// been applied.
    TextDocumentIdentifier textDocument;

    /// The actual content changes.
    std::vector<TextDocumentContentChangeEvent> contentChanges;

    /// Forces diagnostics to be generated, or to not be generated, for this
    /// version of the file. If not set, diagnostics are eventually consistent:
    /// either they will be provided for this version or some subsequent one.
    /// This is a clangd extension.
    //option<bool> wantDiagnostics;
};
JSON_SERIALIZE(DidChangeTextDocumentParams, MAP_JSON(MAP_KEY(textDocument), MAP_KEY(contentChanges)/*, MAP_KEY(wantDiagnostics)*/), {});

enum class FileChangeType {
    /// The file got created.
            Created = 1,
    /// The file got changed.
            Changed = 2,
    /// The file got deleted.
            Deleted = 3
};
struct FileEvent {
    /// The file's URI.
    DocumentUri uri;
    /// The change type.
    FileChangeType type = FileChangeType::Created;
};
JSON_SERIALIZE(FileEvent, MAP_JSON(MAP_KEY(uri), MAP_KEY(type)), {});

struct DidChangeWatchedFilesParams {
    /// The actual file events.
    std::vector<FileEvent> changes;
};
JSON_SERIALIZE(DidChangeWatchedFilesParams, MAP_JSON(MAP_KEY(changes)), {});

struct DidChangeConfigurationParams {
    ConfigurationSettings settings;
};
JSON_SERIALIZE(DidChangeConfigurationParams, MAP_JSON(MAP_KEY(settings)), {});

struct DocumentRangeFormattingParams {
    /// The document to format.
    TextDocumentIdentifier textDocument;

    /// The range to format
    Range range;
};
JSON_SERIALIZE(DocumentRangeFormattingParams, MAP_JSON(MAP_KEY(textDocument), MAP_KEY(range)), {});

struct DocumentOnTypeFormattingParams {
    /// The document to format.
    TextDocumentIdentifier textDocument;

    /// The position at which this request was sent.
    Position position;

    /// The character that has been typed.
    TextType ch;
};
JSON_SERIALIZE(DocumentOnTypeFormattingParams, MAP_JSON(MAP_KEY(textDocument), MAP_KEY(position), MAP_KEY(ch)), {});

struct FoldingRangeParams {
    /// The document to format.
    TextDocumentIdentifier textDocument;
};
JSON_SERIALIZE(FoldingRangeParams, MAP_JSON(MAP_KEY(textDocument)), {});

enum class FoldingRangeKind {
    Comment,
    Imports,
    Region,
};
NLOHMANN_JSON_SERIALIZE_ENUM(FoldingRangeKind, {
    {FoldingRangeKind::Comment, "comment"},
    {FoldingRangeKind::Imports, "imports"},
    {FoldingRangeKind::Region, "region"}
})

struct FoldingRange {
    /**
     * The zero-based line number from where the folded range starts.
     */
    int startLine;
    /**
     * The zero-based character offset from where the folded range starts.
     * If not defined, defaults to the length of the start line.
     */
    int startCharacter;
    /**
     * The zero-based line number where the folded range ends.
     */
    int endLine;
    /**
     * The zero-based character offset before the folded range ends.
     * If not defined, defaults to the length of the end line.
     */
    int endCharacter;

    FoldingRangeKind kind;
};
JSON_SERIALIZE(FoldingRange, {}, {
    FROM_KEY(startLine);
    FROM_KEY(startCharacter);
    FROM_KEY(endLine);
    FROM_KEY(endCharacter);
    FROM_KEY(kind);
});

struct SelectionRangeParams {
    /// The document to format.
    TextDocumentIdentifier textDocument;
    std::vector<Position> positions;
};
JSON_SERIALIZE(SelectionRangeParams, MAP_JSON(MAP_KEY(textDocument), MAP_KEY(positions)), {});

struct SelectionRange {
    Range range;
    std::unique_ptr<SelectionRange> parent;
};
JSON_SERIALIZE(SelectionRange, {}, {
    FROM_KEY(range);
    if (j.contains("parent")) {
        value.parent = std::make_unique<SelectionRange>();
        j.at("parent").get_to(*value.parent);
    }
});

struct DocumentFormattingParams {
    /// The document to format.
    TextDocumentIdentifier textDocument;
};
JSON_SERIALIZE(DocumentFormattingParams, MAP_JSON(MAP_KEY(textDocument)), {});

struct DocumentSymbolParams {
    // The text document to find symbols in.
    TextDocumentIdentifier textDocument;
};
JSON_SERIALIZE(DocumentSymbolParams, MAP_JSON(MAP_KEY(textDocument)), {});

struct DiagnosticRelatedInformation {
    /// The location of this related diagnostic information.
    Location location;
    /// The message of this related diagnostic information.
    std::string message;
};
JSON_SERIALIZE(DiagnosticRelatedInformation, MAP_JSON(MAP_KEY(location), MAP_KEY(message)), {FROM_KEY(location);FROM_KEY(message);});
struct CodeAction;

struct Diagnostic {
    /// The range at which the message applies.
    Range range;

    /// The diagnostic's severity. Can be omitted. If omitted it is up to the
    /// client to interpret diagnostics as error, warning, info or hint.
    int severity = 0;

    /// The diagnostic's code. Can be omitted.
    std::string code;

    /// A human-readable string describing the source of this
    /// diagnostic, e.g. 'typescript' or 'super lint'.
    std::string source;

    /// The diagnostic's message.
    std::string message;

    /// An array of related diagnostic information, e.g. when symbol-names within
    /// a scope collide all definitions can be marked via this property.
    option<std::vector<DiagnosticRelatedInformation>> relatedInformation;

    /// The diagnostic's category. Can be omitted.
    /// An LSP extension that's used to send the name of the category over to the
    /// client. The category typically describes the compilation stage during
    /// which the issue was produced, e.g. "Semantic Issue" or "Parse Issue".
    option<std::string> category;

    /// Clangd extension: code actions related to this diagnostic.
    /// Only with capability textDocument.publishDiagnostics.codeActionsInline.
    /// (These actions can also be obtained using textDocument/codeAction).
    option<std::vector<CodeAction>> codeActions;
};
JSON_SERIALIZE(Diagnostic, {/*NOT REQUIRED*/},{FROM_KEY(range);FROM_KEY(code);FROM_KEY(source);FROM_KEY(message);
                FROM_KEY(relatedInformation);FROM_KEY(category);FROM_KEY(codeActions);});

struct PublishDiagnosticsParams {
    /**
     * The URI for which diagnostic information is reported.
     */
    std::string uri;
    /**
	 * An array of diagnostic information items.
	 */
    std::vector<Diagnostic> diagnostics;
};
JSON_SERIALIZE(PublishDiagnosticsParams, {}, {FROM_KEY(uri);FROM_KEY(diagnostics);});

struct CodeActionContext {
    /// An array of diagnostics.
    std::vector<Diagnostic> diagnostics;
};
JSON_SERIALIZE(CodeActionContext, MAP_JSON(MAP_KEY(diagnostics)), {});

struct CodeActionParams {
    /// The document in which the command was invoked.
    TextDocumentIdentifier textDocument;

    /// The range for which the command was invoked.
    Range range;

    /// Context carrying additional information.
    CodeActionContext context;
};
JSON_SERIALIZE(CodeActionParams, MAP_JSON(MAP_KEY(textDocument), MAP_KEY(range), MAP_KEY(context)), {});

struct WorkspaceEdit {
    /// Holds changes to existing resources.
    option<std::map<std::string, std::vector<TextEdit>>> changes;

    /// Note: "documentChanges" is not currently used because currently there is
    /// no support for versioned edits.
};
JSON_SERIALIZE(WorkspaceEdit, MAP_JSON(MAP_KEY(changes)), {FROM_KEY(changes);});

struct TweakArgs {
    /// A file provided by the client on a textDocument/codeAction request.
    std::string file;
    /// A selection provided by the client on a textDocument/codeAction request.
    Range selection;
    /// ID of the tweak that should be executed. Corresponds to Tweak::id().
    std::string tweakID;
};
JSON_SERIALIZE(TweakArgs, MAP_JSON(MAP_KEY(file), MAP_KEY(selection), MAP_KEY(tweakID)), {FROM_KEY(file);FROM_KEY(selection);FROM_KEY(tweakID);});

struct ExecuteCommandParams {
    std::string command;
    // Arguments
    option<WorkspaceEdit> workspaceEdit;
    option<TweakArgs> tweakArgs;
};
JSON_SERIALIZE(ExecuteCommandParams, MAP_JSON(MAP_KEY(command), MAP_KEY(workspaceEdit), MAP_KEY(tweakArgs)), {});

struct LspCommand : public ExecuteCommandParams {
    std::string title;
};
JSON_SERIALIZE(LspCommand, MAP_JSON(MAP_KEY(command), MAP_KEY(workspaceEdit), MAP_KEY(tweakArgs), MAP_KEY(title)),
        {FROM_KEY(command);FROM_KEY(workspaceEdit);FROM_KEY(tweakArgs);FROM_KEY(title);});

struct CodeAction {
    /// A short, human-readable, title for this code action.
    std::string title;

    /// The kind of the code action.
    /// Used to filter code actions.
    option<std::string> kind;
    /// The diagnostics that this code action resolves.
    option<std::vector<Diagnostic>> diagnostics;

    /// The workspace edit this code action performs.
    option<WorkspaceEdit> edit;

    /// A command this code action executes. If a code action provides an edit
    /// and a command, first the edit is executed and then the command.
    option<LspCommand> command;
};
JSON_SERIALIZE(CodeAction, MAP_JSON(MAP_KEY(title), MAP_KEY(kind), MAP_KEY(diagnostics), MAP_KEY(edit), MAP_KEY(command)),
        {FROM_KEY(title);FROM_KEY(kind);FROM_KEY(diagnostics);FROM_KEY(edit);FROM_KEY(command)});

struct SymbolInformation {
    /// The name of this symbol.
    std::string name;
    /// The kind of this symbol.
    SymbolKind kind = SymbolKind::Class;
    /// The location of this symbol.
    Location location;
    /// The name of the symbol containing this symbol.
    std::string containerName;
};
JSON_SERIALIZE(SymbolInformation, MAP_JSON(MAP_KEY(name), MAP_KEY(kind), MAP_KEY(location), MAP_KEY(containerName)), {FROM_KEY(name);FROM_KEY(kind);FROM_KEY(location);FROM_KEY(containerName)});

struct SymbolDetails {
    TextType name;
    TextType containerName;
    /// Unified Symbol Resolution identifier
    /// This is an opaque string uniquely identifying a symbol.
    /// Unlike SymbolID, it is variable-length and somewhat human-readable.
    /// It is a common representation across several clang tools.
    /// (See USRGeneration.h)
    TextType USR;
    option<TextType> ID;
};

struct WorkspaceSymbolParams {
    /// A non-empty query string
    TextType query;
};
JSON_SERIALIZE(WorkspaceSymbolParams, MAP_JSON(MAP_KEY(query)), {});

struct ApplyWorkspaceEditParams {
    WorkspaceEdit edit;
};
JSON_SERIALIZE(ApplyWorkspaceEditParams, MAP_JSON(MAP_KEY(edit)), {});

struct TextDocumentPositionParams {
    /// The text document.
    TextDocumentIdentifier textDocument;

    /// The position inside the text document.
    Position position;
};
JSON_SERIALIZE(TextDocumentPositionParams, MAP_JSON(MAP_KEY(textDocument), MAP_KEY(position)), {});

enum class CompletionTriggerKind {
    /// Completion was triggered by typing an identifier (24x7 code
    /// complete), manual invocation (e.g Ctrl+Space) or via API.
            Invoked = 1,
    /// Completion was triggered by a trigger character specified by
    /// the `triggerCharacters` properties of the `CompletionRegistrationOptions`.
            TriggerCharacter = 2,
    /// Completion was re-triggered as the current completion list is incomplete.
            TriggerTriggerForIncompleteCompletions = 3
};
struct CompletionContext {
    /// How the completion was triggered.
    CompletionTriggerKind triggerKind = CompletionTriggerKind::Invoked;
    /// The trigger character (a single character) that has trigger code complete.
    /// Is undefined if `triggerKind !== CompletionTriggerKind.TriggerCharacter`
    option<TextType> triggerCharacter;
};
JSON_SERIALIZE(CompletionContext, MAP_JSON(MAP_KEY(triggerKind), MAP_KEY(triggerCharacter)), {});

struct CompletionParams : TextDocumentPositionParams {
    option<CompletionContext> context;
};
JSON_SERIALIZE(CompletionParams, MAP_JSON(MAP_KEY(context), MAP_KEY(textDocument), MAP_KEY(position)), {});

struct MarkupContent {
    MarkupKind kind = MarkupKind::PlainText;
    std::string value;
};
JSON_SERIALIZE(MarkupContent, {}, {FROM_KEY(kind);FROM_KEY(value)});

struct Hover {
    /// The hover's content
    MarkupContent contents;

    /// An optional range is a range inside a text document
    /// that is used to visualize a hover, e.g. by changing the background color.
    option<Range> range;
};
JSON_SERIALIZE(Hover, {}, {FROM_KEY(contents);FROM_KEY(range)});

enum class InsertTextFormat {
    Missing = 0,
    /// The primary text to be inserted is treated as a plain string.
            PlainText = 1,
    /// The primary text to be inserted is treated as a snippet.
    ///
    /// A snippet can define tab stops and placeholders with `$1`, `$2`
    /// and `${3:foo}`. `$0` defines the final tab stop, it defaults to the end
    /// of the snippet. Placeholders with equal identifiers are linked, that is
    /// typing in one will update others too.
    ///
    /// See also:
    /// https//github.com/Microsoft/vscode/blob/master/src/vs/editor/contrib/snippet/common/snippet.md
            Snippet = 2,
};
struct CompletionItem {
    /// The label of this completion item. By default also the text that is
    /// inserted when selecting this completion.
    std::string label;

    /// The kind of this completion item. Based of the kind an icon is chosen by
    /// the editor.
    CompletionItemKind kind = CompletionItemKind::Missing;

    /// A human-readable string with additional information about this item, like
    /// type or symbol information.
    std::string detail;

    /// A human-readable string that represents a doc-comment.
    std::string documentation;

    /// A string that should be used when comparing this item with other items.
    /// When `falsy` the label is used.
    std::string sortText;

    /// A string that should be used when filtering a set of completion items.
    /// When `falsy` the label is used.
    std::string filterText;

    /// A string that should be inserted to a document when selecting this
    /// completion. When `falsy` the label is used.
    std::string insertText;

    /// The format of the insert text. The format applies to both the `insertText`
    /// property and the `newText` property of a provided `textEdit`.
    InsertTextFormat insertTextFormat = InsertTextFormat::Missing;

    /// An edit which is applied to a document when selecting this completion.
    /// When an edit is provided `insertText` is ignored.
    ///
    /// Note: The range of the edit must be a single line range and it must
    /// contain the position at which completion has been requested.
    TextEdit textEdit;

    /// An optional array of additional text edits that are applied when selecting
    /// this completion. Edits must not overlap with the main edit nor with
    /// themselves.
    std::vector<TextEdit> additionalTextEdits;

    /// Indicates if this item is deprecated.
    bool deprecated = false;

    // TODO(krasimir): The following optional fields defined by the language
    // server protocol are unsupported:
    //
    // data?: any - A data entry field that is preserved on a completion item
    //              between a completion and a completion resolve request.
};
JSON_SERIALIZE(CompletionItem, {}, {
    FROM_KEY(label);
    FROM_KEY(kind);
    FROM_KEY(detail);
    FROM_KEY(documentation);
    FROM_KEY(sortText);
    FROM_KEY(filterText);
    FROM_KEY(insertText);
    FROM_KEY(insertTextFormat);
    FROM_KEY(textEdit);
    FROM_KEY(additionalTextEdits);
});

struct CompletionList {
    /// The list is not complete. Further typing should result in recomputing the
    /// list.
    bool isIncomplete = false;

    /// The completion items.
    std::vector<CompletionItem> items;
};
JSON_SERIALIZE(CompletionList, {}, {
    FROM_KEY(isIncomplete);
    FROM_KEY(items);
});

struct ParameterInformation {

    /// The label of this parameter. Ignored when labelOffsets is set.
    std::string labelString;

    /// Inclusive start and exclusive end offsets withing the containing signature
    /// label.
    /// Offsets are computed by lspLength(), which counts UTF-16 code units by
    /// default but that can be overriden, see its documentation for details.
    option<std::pair<unsigned, unsigned>> labelOffsets;

    /// The documentation of this parameter. Optional.
    std::string documentation;
};
JSON_SERIALIZE(ParameterInformation, {}, {
    FROM_KEY(labelString);
    FROM_KEY(labelOffsets);
    FROM_KEY(documentation);
});
struct SignatureInformation {

    /// The label of this signature. Mandatory.
    std::string label;

    /// The documentation of this signature. Optional.
    std::string documentation;

    /// The parameters of this signature.
    std::vector<ParameterInformation> parameters;
};
JSON_SERIALIZE(SignatureInformation, {}, {
    FROM_KEY(label);
    FROM_KEY(documentation);
    FROM_KEY(parameters);
});
struct SignatureHelp {
    /// The resulting signatures.
    std::vector<SignatureInformation> signatures;
    /// The active signature.
    int activeSignature = 0;
    /// The active parameter of the active signature.
    int activeParameter = 0;
    /// Position of the start of the argument list, including opening paren. e.g.
    /// foo("first arg",   "second arg",
    ///    ^-argListStart   ^-cursor
    /// This is a clangd-specific extension, it is only available via C++ API and
    /// not currently serialized for the LSP.
    Position argListStart;
};
JSON_SERIALIZE(SignatureHelp, {}, {
    FROM_KEY(signatures);
    FROM_KEY(activeParameter);
    FROM_KEY(argListStart);
});

struct RenameParams {
    /// The document that was opened.
    TextDocumentIdentifier textDocument;

    /// The position at which this request was sent.
    Position position;

    /// The new name of the symbol.
    std::string newName;
};
JSON_SERIALIZE(RenameParams, MAP_JSON(MAP_KEY(textDocument), MAP_KEY(position), MAP_KEY(newName)), {});

enum class DocumentHighlightKind { Text = 1, Read = 2, Write = 3 };

struct DocumentHighlight {
    /// The range this highlight applies to.
    Range range;
    /// The highlight kind, default is DocumentHighlightKind.Text.
    DocumentHighlightKind kind = DocumentHighlightKind::Text;
    friend bool operator<(const DocumentHighlight &LHS,
                          const DocumentHighlight &RHS) {
        int LHSKind = static_cast<int>(LHS.kind);
        int RHSKind = static_cast<int>(RHS.kind);
        return std::tie(LHS.range, LHSKind) < std::tie(RHS.range, RHSKind);
    }
    friend bool operator==(const DocumentHighlight &LHS,
                           const DocumentHighlight &RHS) {
        return LHS.kind == RHS.kind && LHS.range == RHS.range;
    }
};
enum class TypeHierarchyDirection { Children = 0, Parents = 1, Both = 2 };

struct TypeHierarchyParams : public TextDocumentPositionParams {
    /// The hierarchy levels to resolve. `0` indicates no level.
    int resolve = 0;

    /// The direction of the hierarchy levels to resolve.
    TypeHierarchyDirection direction = TypeHierarchyDirection::Parents;
};
JSON_SERIALIZE(TypeHierarchyParams, MAP_JSON(MAP_KEY(resolve), MAP_KEY(direction), MAP_KEY(textDocument), MAP_KEY(position)), {});

struct TypeHierarchyItem {
    /// The human readable name of the hierarchy item.
    std::string name;

    /// Optional detail for the hierarchy item. It can be, for instance, the
    /// signature of a function or method.
    option<std::string> detail;

    /// The kind of the hierarchy item. For instance, class or interface.
    SymbolKind kind;

    /// `true` if the hierarchy item is deprecated. Otherwise, `false`.
    bool deprecated;

    /// The URI of the text document where this type hierarchy item belongs to.
    DocumentUri uri;

    /// The range enclosing this type hierarchy item not including
    /// leading/trailing whitespace but everything else like comments. This
    /// information is typically used to determine if the client's cursor is
    /// inside the type hierarch item to reveal in the symbol in the UI.
    Range range;

    /// The range that should be selected and revealed when this type hierarchy
    /// item is being picked, e.g. the name of a function. Must be contained by
    /// the `range`.
    Range selectionRange;

    /// If this type hierarchy item is resolved, it contains the direct parents.
    /// Could be empty if the item does not have direct parents. If not defined,
    /// the parents have not been resolved yet.
    option<std::vector<TypeHierarchyItem>> parents;

    /// If this type hierarchy item is resolved, it contains the direct children
    /// of the current item. Could be empty if the item does not have any
    /// descendants. If not defined, the children have not been resolved.
    option<std::vector<TypeHierarchyItem>> children;

    /// The protocol has a slot here for an optional 'data' filed, which can
    /// be used to identify a type hierarchy item in a resolve request. We don't
    /// need this (the item itself is sufficient to identify what to resolve)
    /// so don't declare it.
};

struct ReferenceParams : public TextDocumentPositionParams {
    // For now, no options like context.includeDeclaration are supported.
};
JSON_SERIALIZE(ReferenceParams, MAP_JSON(MAP_KEY(textDocument), MAP_KEY(position)), {});
struct FileStatus {
    /// The text document's URI.
    DocumentUri uri;
    /// The human-readable string presents the current state of the file, can be
    /// shown in the UI (e.g. status bar).
    TextType state;
    // FIXME: add detail messages.
};

// Parameters for the document link request.
struct DocumentLinkParams {
  /// The document to provide document links for.
  TextDocumentIdentifier textDocument;
};
JSON_SERIALIZE(DocumentLinkParams, MAP_JSON(MAP_KEY(textDocument)), {});



// A range in a text document that links to an internal or external resource,
// like another text document or a web site.
struct DocumentLink {
  // The range this link applies to.
  Range range;

  // The uri this link points to. If missing a resolve request is sent later.
  std::string target;

  // TODO(forster): The following optional fields defined by the language
  // server protocol are unsupported:
  //
  // data?: any - A data entry field that is preserved on a document link
  //              between a DocumentLinkRequest and a
  //              DocumentLinkResolveRequest.
  friend bool operator==(const DocumentLink &LHS, const DocumentLink &RHS) {
    return LHS.range == RHS.range && LHS.target == RHS.target;
  }

  friend bool operator!=(const DocumentLink &LHS, const DocumentLink &RHS) {
    return !(LHS == RHS);
  }
};
JSON_SERIALIZE(DocumentLink, MAP_JSON(MAP_KEY(range), MAP_KEY(target)), {FROM_KEY(range);FROM_KEY(target);});

#endif //LSP_PROTOCOL_H
