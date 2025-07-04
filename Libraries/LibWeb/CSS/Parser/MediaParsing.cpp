/*
 * Copyright (c) 2018-2022, Andreas Kling <andreas@ladybird.org>
 * Copyright (c) 2020-2021, the SerenityOS developers.
 * Copyright (c) 2021-2025, Sam Atkins <sam@ladybird.org>
 * Copyright (c) 2021, Tobias Christiansen <tobyase@serenityos.org>
 * Copyright (c) 2022, MacDue <macdue@dueutil.tech>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <AK/Debug.h>
#include <LibWeb/CSS/CSSMediaRule.h>
#include <LibWeb/CSS/CSSNestedDeclarations.h>
#include <LibWeb/CSS/CalculatedOr.h>
#include <LibWeb/CSS/MediaList.h>
#include <LibWeb/CSS/MediaQuery.h>
#include <LibWeb/CSS/Parser/Parser.h>

namespace Web::CSS::Parser {

Vector<NonnullRefPtr<MediaQuery>> Parser::parse_as_media_query_list()
{
    return parse_a_media_query_list(m_token_stream);
}

template<typename T>
Vector<NonnullRefPtr<MediaQuery>> Parser::parse_a_media_query_list(TokenStream<T>& tokens)
{
    // https://www.w3.org/TR/mediaqueries-4/#mq-list

    // AD-HOC: Ignore whitespace-only queries
    // to make `@media {..}` equivalent to `@media all {..}`
    tokens.discard_whitespace();
    if (!tokens.has_next_token())
        return {};

    auto comma_separated_lists = parse_a_comma_separated_list_of_component_values(tokens);

    AK::Vector<NonnullRefPtr<MediaQuery>> media_queries;
    for (auto& media_query_parts : comma_separated_lists) {
        auto stream = TokenStream(media_query_parts);
        media_queries.append(parse_media_query(stream));
    }

    return media_queries;
}

RefPtr<MediaQuery> Parser::parse_as_media_query()
{
    // https://www.w3.org/TR/cssom-1/#parse-a-media-query
    auto media_query_list = parse_as_media_query_list();
    if (media_query_list.is_empty())
        return MediaQuery::create_not_all();
    if (media_query_list.size() == 1)
        return media_query_list.first();
    return nullptr;
}

// `<media-query>`, https://www.w3.org/TR/mediaqueries-4/#typedef-media-query
NonnullRefPtr<MediaQuery> Parser::parse_media_query(TokenStream<ComponentValue>& tokens)
{
    // `<media-query> = <media-condition>
    //                | [ not | only ]? <media-type> [ and <media-condition-without-or> ]?`

    // `[ not | only ]?`, Returns whether to negate the query
    auto parse_initial_modifier = [](auto& tokens) -> Optional<bool> {
        auto transaction = tokens.begin_transaction();
        tokens.discard_whitespace();
        auto& token = tokens.consume_a_token();
        if (!token.is(Token::Type::Ident))
            return {};

        auto ident = token.token().ident();
        if (ident.equals_ignoring_ascii_case("not"sv)) {
            transaction.commit();
            return true;
        }
        if (ident.equals_ignoring_ascii_case("only"sv)) {
            transaction.commit();
            return false;
        }
        return {};
    };

    auto invalid_media_query = [&]() {
        // "A media query that does not match the grammar in the previous section must be replaced by `not all`
        // during parsing." - https://www.w3.org/TR/mediaqueries-5/#error-handling
        if constexpr (CSS_PARSER_DEBUG) {
            dbgln("Invalid media query:");
            tokens.dump_all_tokens();
        }
        return MediaQuery::create_not_all();
    };

    auto media_query = MediaQuery::create();
    tokens.discard_whitespace();

    // `<media-condition>`
    if (auto media_condition = parse_media_condition(tokens)) {
        tokens.discard_whitespace();
        if (tokens.has_next_token())
            return invalid_media_query();
        media_query->m_media_condition = media_condition.release_nonnull();
        return media_query;
    }

    // `[ not | only ]?`
    if (auto modifier = parse_initial_modifier(tokens); modifier.has_value()) {
        media_query->m_negated = modifier.value();
        tokens.discard_whitespace();
    }

    // `<media-type>`
    if (auto media_type = parse_media_type(tokens); media_type.has_value()) {
        media_query->m_media_type = media_type.release_value();
        tokens.discard_whitespace();
    } else {
        // https://drafts.csswg.org/mediaqueries-4/#error-handling
        // A media query that does not match the grammar in the previous section must be replaced by not all during parsing.
        return invalid_media_query();
    }

    if (!tokens.has_next_token())
        return media_query;

    // `[ and <media-condition-without-or> ]?`
    if (auto const& maybe_and = tokens.consume_a_token(); maybe_and.is_ident("and"sv)) {
        if (auto media_condition = parse_media_condition(tokens)) {
            // "or" is disallowed at the top level
            if (is<BooleanOrExpression>(*media_condition))
                return invalid_media_query();

            tokens.discard_whitespace();
            if (tokens.has_next_token())
                return invalid_media_query();
            media_query->m_media_condition = move(media_condition);
            return media_query;
        }
        return invalid_media_query();
    }

    return invalid_media_query();
}

// `<media-condition>`, https://www.w3.org/TR/mediaqueries-4/#typedef-media-condition
OwnPtr<BooleanExpression> Parser::parse_media_condition(TokenStream<ComponentValue>& tokens)
{
    return parse_boolean_expression(tokens, MatchResult::Unknown, [this](TokenStream<ComponentValue>& tokens) -> OwnPtr<BooleanExpression> {
        return parse_media_feature(tokens);
    });
}

// `<media-feature>`, https://www.w3.org/TR/mediaqueries-4/#typedef-media-feature
OwnPtr<MediaFeature> Parser::parse_media_feature(TokenStream<ComponentValue>& tokens)
{
    // `[ <mf-plain> | <mf-boolean> | <mf-range> ]`
    tokens.discard_whitespace();

    // `<mf-name> = <ident>`
    struct MediaFeatureName {
        enum Type {
            Normal,
            Min,
            Max
        } type;
        MediaFeatureID id;
    };
    auto parse_mf_name = [](auto& tokens, bool allow_min_max_prefix) -> Optional<MediaFeatureName> {
        auto transaction = tokens.begin_transaction();
        auto& token = tokens.consume_a_token();
        if (token.is(Token::Type::Ident)) {
            auto name = token.token().ident();
            if (auto id = media_feature_id_from_string(name); id.has_value()) {
                transaction.commit();
                return MediaFeatureName { MediaFeatureName::Type::Normal, id.value() };
            }

            if (allow_min_max_prefix && (name.starts_with_bytes("min-"sv, CaseSensitivity::CaseInsensitive) || name.starts_with_bytes("max-"sv, CaseSensitivity::CaseInsensitive))) {
                auto adjusted_name = name.bytes_as_string_view().substring_view(4);
                if (auto id = media_feature_id_from_string(adjusted_name); id.has_value() && media_feature_type_is_range(id.value())) {
                    transaction.commit();
                    return MediaFeatureName {
                        name.starts_with_bytes("min-"sv, CaseSensitivity::CaseInsensitive) ? MediaFeatureName::Type::Min : MediaFeatureName::Type::Max,
                        id.value()
                    };
                }
            }
        }
        return {};
    };

    // `<mf-boolean> = <mf-name>`
    auto parse_mf_boolean = [&](auto& tokens) -> OwnPtr<MediaFeature> {
        auto transaction = tokens.begin_transaction();
        tokens.discard_whitespace();

        if (auto maybe_name = parse_mf_name(tokens, false); maybe_name.has_value()) {
            tokens.discard_whitespace();
            if (!tokens.has_next_token()) {
                transaction.commit();
                return MediaFeature::boolean(maybe_name->id);
            }
        }

        return {};
    };

    // `<mf-plain> = <mf-name> : <mf-value>`
    auto parse_mf_plain = [&](auto& tokens) -> OwnPtr<MediaFeature> {
        auto transaction = tokens.begin_transaction();
        tokens.discard_whitespace();

        if (auto maybe_name = parse_mf_name(tokens, true); maybe_name.has_value()) {
            tokens.discard_whitespace();
            if (tokens.consume_a_token().is(Token::Type::Colon)) {
                tokens.discard_whitespace();
                if (auto maybe_value = parse_media_feature_value(maybe_name->id, tokens); maybe_value.has_value()) {
                    tokens.discard_whitespace();
                    if (!tokens.has_next_token()) {
                        transaction.commit();
                        switch (maybe_name->type) {
                        case MediaFeatureName::Type::Normal:
                            return MediaFeature::plain(maybe_name->id, maybe_value.release_value());
                        case MediaFeatureName::Type::Min:
                            return MediaFeature::min(maybe_name->id, maybe_value.release_value());
                        case MediaFeatureName::Type::Max:
                            return MediaFeature::max(maybe_name->id, maybe_value.release_value());
                        }
                        VERIFY_NOT_REACHED();
                    }
                }
            }
        }
        return {};
    };

    // `<mf-lt> = '<' '='?
    //  <mf-gt> = '>' '='?
    //  <mf-eq> = '='
    //  <mf-comparison> = <mf-lt> | <mf-gt> | <mf-eq>`
    auto parse_comparison = [](auto& tokens) -> Optional<MediaFeature::Comparison> {
        auto transaction = tokens.begin_transaction();
        tokens.discard_whitespace();

        auto& first = tokens.consume_a_token();
        if (first.is(Token::Type::Delim)) {
            auto first_delim = first.token().delim();
            if (first_delim == '=') {
                transaction.commit();
                return MediaFeature::Comparison::Equal;
            }
            if (first_delim == '<') {
                auto& second = tokens.next_token();
                if (second.is_delim('=')) {
                    tokens.discard_a_token();
                    transaction.commit();
                    return MediaFeature::Comparison::LessThanOrEqual;
                }
                transaction.commit();
                return MediaFeature::Comparison::LessThan;
            }
            if (first_delim == '>') {
                auto& second = tokens.next_token();
                if (second.is_delim('=')) {
                    tokens.discard_a_token();
                    transaction.commit();
                    return MediaFeature::Comparison::GreaterThanOrEqual;
                }
                transaction.commit();
                return MediaFeature::Comparison::GreaterThan;
            }
        }

        return {};
    };

    auto comparisons_match = [](MediaFeature::Comparison a, MediaFeature::Comparison b) -> bool {
        switch (a) {
        case MediaFeature::Comparison::Equal:
            return b == MediaFeature::Comparison::Equal;
        case MediaFeature::Comparison::LessThan:
        case MediaFeature::Comparison::LessThanOrEqual:
            return b == MediaFeature::Comparison::LessThan || b == MediaFeature::Comparison::LessThanOrEqual;
        case MediaFeature::Comparison::GreaterThan:
        case MediaFeature::Comparison::GreaterThanOrEqual:
            return b == MediaFeature::Comparison::GreaterThan || b == MediaFeature::Comparison::GreaterThanOrEqual;
        }
        VERIFY_NOT_REACHED();
    };

    // `<mf-range> = <mf-name> <mf-comparison> <mf-value>
    //             | <mf-value> <mf-comparison> <mf-name>
    //             | <mf-value> <mf-lt> <mf-name> <mf-lt> <mf-value>
    //             | <mf-value> <mf-gt> <mf-name> <mf-gt> <mf-value>`
    auto parse_mf_range = [&](auto& tokens) -> OwnPtr<MediaFeature> {
        auto transaction = tokens.begin_transaction();
        tokens.discard_whitespace();

        // `<mf-name> <mf-comparison> <mf-value>`
        // NOTE: We have to check for <mf-name> first, since all <mf-name>s will also parse as <mf-value>.
        if (auto maybe_name = parse_mf_name(tokens, false); maybe_name.has_value()) {
            tokens.discard_whitespace();
            if (auto maybe_comparison = parse_comparison(tokens); maybe_comparison.has_value()) {
                tokens.discard_whitespace();
                if (auto maybe_value = parse_media_feature_value(maybe_name->id, tokens); maybe_value.has_value()) {
                    tokens.discard_whitespace();
                    if (!tokens.has_next_token() && !maybe_value->is_ident()) {
                        transaction.commit();
                        return MediaFeature::half_range(maybe_name->id, maybe_comparison.release_value(), maybe_value.release_value());
                    }
                }
            }
        }

        //  `<mf-value> <mf-comparison> <mf-name>
        // | <mf-value> <mf-lt> <mf-name> <mf-lt> <mf-value>
        // | <mf-value> <mf-gt> <mf-name> <mf-gt> <mf-value>`
        // NOTE: To parse the first value, we need to first find and parse the <mf-name> so we know what value types to parse.
        //       To allow for <mf-value> to be any number of tokens long, we scan forward until we find a comparison, and then
        //       treat the next non-whitespace token as the <mf-name>, which should be correct as long as they don't add a value
        //       type that can include a comparison in it. :^)
        Optional<MediaFeatureName> maybe_name;
        {
            // This transaction is never committed, we just use it to rewind automatically.
            auto temp_transaction = tokens.begin_transaction();
            while (tokens.has_next_token() && !maybe_name.has_value()) {
                if (auto maybe_comparison = parse_comparison(tokens); maybe_comparison.has_value()) {
                    // We found a comparison, so the next non-whitespace token should be the <mf-name>
                    tokens.discard_whitespace();
                    maybe_name = parse_mf_name(tokens, false);
                    break;
                }
                tokens.discard_a_token();
                tokens.discard_whitespace();
            }
        }

        // Now, we can parse the range properly.
        if (maybe_name.has_value()) {
            if (auto maybe_left_value = parse_media_feature_value(maybe_name->id, tokens); maybe_left_value.has_value()) {
                tokens.discard_whitespace();
                if (auto maybe_left_comparison = parse_comparison(tokens); maybe_left_comparison.has_value()) {
                    tokens.discard_whitespace();
                    tokens.discard_a_token(); // The <mf-name> which we already parsed above.
                    tokens.discard_whitespace();

                    if (!tokens.has_next_token()) {
                        transaction.commit();
                        return MediaFeature::half_range(maybe_left_value.release_value(), maybe_left_comparison.release_value(), maybe_name->id);
                    }

                    if (auto maybe_right_comparison = parse_comparison(tokens); maybe_right_comparison.has_value()) {
                        tokens.discard_whitespace();
                        if (auto maybe_right_value = parse_media_feature_value(maybe_name->id, tokens); maybe_right_value.has_value()) {
                            tokens.discard_whitespace();
                            // For this to be valid, the following must be true:
                            // - Comparisons must either both be >/>= or both be </<=.
                            // - Neither comparison can be `=`.
                            // - Neither value can be an ident.
                            auto left_comparison = maybe_left_comparison.release_value();
                            auto right_comparison = maybe_right_comparison.release_value();

                            if (!tokens.has_next_token()
                                && comparisons_match(left_comparison, right_comparison)
                                && left_comparison != MediaFeature::Comparison::Equal
                                && !maybe_left_value->is_ident() && !maybe_right_value->is_ident()) {
                                transaction.commit();
                                return MediaFeature::range(maybe_left_value.release_value(), left_comparison, maybe_name->id, right_comparison, maybe_right_value.release_value());
                            }
                        }
                    }
                }
            }
        }

        return {};
    };

    if (auto maybe_mf_boolean = parse_mf_boolean(tokens))
        return maybe_mf_boolean.release_nonnull();

    if (auto maybe_mf_plain = parse_mf_plain(tokens))
        return maybe_mf_plain.release_nonnull();

    if (auto maybe_mf_range = parse_mf_range(tokens))
        return maybe_mf_range.release_nonnull();

    return {};
}

Optional<MediaQuery::MediaType> Parser::parse_media_type(TokenStream<ComponentValue>& tokens)
{
    auto transaction = tokens.begin_transaction();
    tokens.discard_whitespace();
    auto const& token = tokens.consume_a_token();

    if (!token.is(Token::Type::Ident))
        return {};

    // https://drafts.csswg.org/mediaqueries-3/#error-handling
    // "However, an exception is made for media types ‘layer’, ‘not’, ‘and’, ‘only’, and ‘or’. Even though they do match
    // the IDENT production, they must not be treated as unknown media types, but rather trigger the malformed query clause."
    if (token.is_ident("layer"sv) || token.is_ident("not"sv) || token.is_ident("and"sv) || token.is_ident("only"sv) || token.is_ident("or"sv))
        return {};

    transaction.commit();

    auto const& ident = token.token().ident();
    return MediaQuery::MediaType {
        .name = ident,
        .known_type = media_type_from_string(ident),
    };
}

static bool is_media_feature_value_token(ComponentValue const& component_value)
{
    if (!component_value.is_token())
        return true;
    switch (component_value.token().type()) {
    case Token::Type::Ident:
    case Token::Type::Function:
    case Token::Type::AtKeyword:
    case Token::Type::Hash:
    case Token::Type::String:
    case Token::Type::BadString:
    case Token::Type::Url:
    case Token::Type::BadUrl:
    case Token::Type::Number:
    case Token::Type::Percentage:
    case Token::Type::Dimension:
    case Token::Type::Whitespace:
    case Token::Type::Comma:
        return true;
    case Token::Type::Delim:
        // FIXME: What list of delimiters should we actually allow here?
        return !first_is_one_of(component_value.token().delim(), static_cast<u32>('<'), static_cast<u32>('>'), static_cast<u32>('='));
    case Token::Type::Invalid:
    case Token::Type::EndOfFile:
    case Token::Type::CDO:
    case Token::Type::CDC:
    case Token::Type::Colon:
    case Token::Type::Semicolon:
    case Token::Type::OpenSquare:
    case Token::Type::CloseSquare:
    case Token::Type::OpenParen:
    case Token::Type::CloseParen:
    case Token::Type::OpenCurly:
    case Token::Type::CloseCurly:
        return false;
    }
    VERIFY_NOT_REACHED();
}

// `<mf-value>`, https://www.w3.org/TR/mediaqueries-4/#typedef-mf-value
Optional<MediaFeatureValue> Parser::parse_media_feature_value(MediaFeatureID media_feature, TokenStream<ComponentValue>& tokens)
{
    {
        auto transaction = tokens.begin_transaction();
        auto value = [this](MediaFeatureID media_feature, TokenStream<ComponentValue>& tokens) -> Optional<MediaFeatureValue> {
            // One branch for each member of the MediaFeatureValueType enum:
            // Identifiers
            if (tokens.next_token().is(Token::Type::Ident)) {
                auto transaction = tokens.begin_transaction();
                tokens.discard_whitespace();
                auto keyword = keyword_from_string(tokens.consume_a_token().token().ident());
                if (keyword.has_value() && media_feature_accepts_keyword(media_feature, keyword.value())) {
                    transaction.commit();
                    return MediaFeatureValue(keyword.value());
                }
            }

            // Boolean (<mq-boolean> in the spec: a 1 or 0)
            if (media_feature_accepts_type(media_feature, MediaFeatureValueType::Boolean)) {
                auto transaction = tokens.begin_transaction();
                tokens.discard_whitespace();
                if (auto integer = parse_integer(tokens); integer.has_value()) {
                    if (integer.value().is_calculated() || integer->value() == 0 || integer->value() == 1) {
                        transaction.commit();
                        return MediaFeatureValue(integer.release_value());
                    }
                }
            }

            // Integer
            if (media_feature_accepts_type(media_feature, MediaFeatureValueType::Integer)) {
                auto transaction = tokens.begin_transaction();
                if (auto integer = parse_integer(tokens); integer.has_value()) {
                    transaction.commit();
                    return MediaFeatureValue(integer.release_value());
                }
            }

            // Length
            if (media_feature_accepts_type(media_feature, MediaFeatureValueType::Length)) {
                auto transaction = tokens.begin_transaction();
                tokens.discard_whitespace();
                if (auto length = parse_length(tokens); length.has_value()) {
                    transaction.commit();
                    return MediaFeatureValue(length.release_value());
                }
            }

            // Ratio
            if (media_feature_accepts_type(media_feature, MediaFeatureValueType::Ratio)) {
                auto transaction = tokens.begin_transaction();
                tokens.discard_whitespace();
                if (auto ratio = parse_ratio(tokens); ratio.has_value()) {
                    transaction.commit();
                    return MediaFeatureValue(ratio.release_value());
                }
            }

            // Resolution
            if (media_feature_accepts_type(media_feature, MediaFeatureValueType::Resolution)) {
                auto transaction = tokens.begin_transaction();
                tokens.discard_whitespace();
                if (auto resolution = parse_resolution(tokens); resolution.has_value()) {
                    transaction.commit();
                    return MediaFeatureValue(resolution.release_value());
                }
            }

            return {};
        }(media_feature, tokens);

        if (value.has_value()) {
            tokens.discard_whitespace();

            // Only returned the value if there are no trailing tokens.
            // Otherwise, the transaction gets reverted and we consume all the value tokens below.
            if (!is_media_feature_value_token(tokens.next_token())) {
                transaction.commit();
                return value.release_value();
            }
        }
    }

    // Parsing failed somehow, so wrap all the tokens into an "unknown" MediaFeatureValue if possible.

    auto transaction = tokens.begin_transaction();
    tokens.discard_whitespace();
    Vector<ComponentValue> unknown_tokens;

    // Consume any tokens that could be part of a value.
    while (tokens.has_next_token()) {
        if (is_media_feature_value_token(tokens.next_token())) {
            unknown_tokens.append(tokens.consume_a_token());
        } else {
            break;
        }
    }

    if (!unknown_tokens.is_empty()) {
        transaction.commit();
        dbgln_if(CSS_PARSER_DEBUG, "Creating unknown media value: `{}`", String::join(""sv, unknown_tokens));
        return MediaFeatureValue(move(unknown_tokens));
    }

    return {};
}

GC::Ptr<CSSMediaRule> Parser::convert_to_media_rule(AtRule const& rule, Nested nested)
{
    // https://drafts.csswg.org/css-conditional-3/#at-media
    // @media <media-query-list> {
    // <rule-list>
    // }
    if (!rule.is_block_rule) {
        dbgln_if(CSS_PARSER_DEBUG, "Failed to parse @media rule: Expected a block.");
        return nullptr;
    }

    auto media_query_tokens = TokenStream { rule.prelude };
    auto media_query_list = parse_a_media_query_list(media_query_tokens);
    auto media_list = MediaList::create(realm(), move(media_query_list));

    GC::RootVector<GC::Ref<CSSRule>> child_rules { realm().heap() };
    for (auto const& child : rule.child_rules_and_lists_of_declarations) {
        child.visit(
            [&](Rule const& rule) {
                if (auto child_rule = convert_to_rule(rule, nested))
                    child_rules.append(*child_rule);
            },
            [&](Vector<Declaration> const& declarations) {
                child_rules.append(CSSNestedDeclarations::create(realm(), *convert_to_style_declaration(declarations)));
            });
    }
    auto rule_list = CSSRuleList::create(realm(), child_rules);
    return CSSMediaRule::create(realm(), media_list, rule_list);
}

}
