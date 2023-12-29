/*
 * Copyright 2023, Andrea Anzani <andrea.anzani@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef protocol_objects_H
#define protocol_objects_H

#include <string>
#include <vector>
#include <tuple>
#include "uri.h"

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

struct TextEdit {
    /// The range of the text document to be manipulated. To insert
    /// text into a document create a range where start === end.
    Range range;

    /// The string to be inserted. For delete operations use an
    /// empty string.
    std::string newText;
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
    //std::string documentation;

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

struct CompletionList {
    /// The list is not complete. Further typing should result in recomputing the
    /// list.
    bool isIncomplete = false;

    /// The completion items.
    std::vector<CompletionItem> items;
};

///// SignatureHelp ////////

struct ParameterInformation {

    /// The label of this parameter. Ignored when labelOffsets is set.
    std::string labelString;

    /// Inclusive start and exclusive end offsets withing the containing signature
    /// label.
    /// Offsets are computed by lspLength(), which counts UTF-16 code units by
    /// default but that can be overriden, see its documentation for details.
    std::pair<unsigned, unsigned> labelOffsets;

    /// The documentation of this parameter. Optional.
    std::string documentation;
};

struct SignatureInformation {

    /// The label of this signature. Mandatory.
    std::string label;

    /// The documentation of this signature. Optional.
    std::string documentation;

    /// The parameters of this signature.
    std::vector<ParameterInformation> parameters;
};

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

struct DiagnosticRelatedInformation {
    /// The location of this related diagnostic information.
    Location location;
    /// The message of this related diagnostic information.
    std::string message;
};

struct CodeAction;

struct Diagnostic {
    /// The range at which the message applies.
    Range range;

    /// The diagnostic's severity. Can be omitted. If omitted it is up to the
    /// client to interpret diagnostics as error, warning, info or hint.
    int severity = 0;

    /// The diagnostic's code. Can be omitted.
	/// Note: this has been removed as can be a string (clangd) or a int (ccls)
	/// we must implement a proper deserializer but because so far is not used,
	/// let's just comment it.

    ///std::string code; //code?: integer | string;

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

struct WorkspaceEdit {
    /// Holds changes to existing resources.
    option<std::map<std::string, std::vector<TextEdit>>> changes;

    /// Note: "documentChanges" is not currently used because currently there is
    /// no support for versioned edits.
};

struct TweakArgs {
    /// A file provided by the client on a textDocument/codeAction request.
    std::string file;
    /// A selection provided by the client on a textDocument/codeAction request.
    Range selection;
    /// ID of the tweak that should be executed. Corresponds to Tweak::id().
    std::string tweakID;
};

struct ExecuteCommandParams {
    std::string command;
    // Arguments
    option<WorkspaceEdit> workspaceEdit;
    option<TweakArgs> tweakArgs;
};

struct LspCommand : public ExecuteCommandParams {
    std::string title;
};

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


#endif // protocol_objects_H
