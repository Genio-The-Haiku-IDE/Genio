void linting_lsp_server_handler::apply_document_changes(

    quick_lint_js::document<lsp_locator>& doc,
    ::simdjson::ondemand::array& changes) {
    	
  for (::simdjson::simdjson_result<::simdjson::ondemand::value> change : changes) {
  	
    std::optional<string8_view> change_text = maybe_make_string_view(change["text"]);
    
    if (!change_text.has_value()) {
      // Ignore invalid change.
      continue;
    }
    
    ::simdjson::ondemand::object raw_range;
    bool is_incremental = change["range"].get(raw_range) == ::simdjson::error_code::SUCCESS;
    
    if (is_incremental) {
      auto start = raw_range["start"];
      std::optional<int> start_line = maybe_get_int(start["line"]);
      std::optional<int> start_character = maybe_get_int(start["character"]);
      auto end = raw_range["end"];
      std::optional<int> end_line = maybe_get_int(end["line"]);
      std::optional<int> end_character = maybe_get_int(end["character"]);
      
      if (!(start_line.has_value() && start_character.has_value() &&
            end_line.has_value() && end_character.has_value())) {
        // Ignore invalid change.
        continue;
      }
      lsp_range range = {
          .start =
              {
                  .line = *start_line,
                  .character = *start_character,
              },
          .end =
              {
                  .line = *end_line,
                  .character = *end_character,
              },
      };
      doc.replace_text(range, *change_text);
    } else {
      doc.set_text(*change_text);
    }
  }
}
