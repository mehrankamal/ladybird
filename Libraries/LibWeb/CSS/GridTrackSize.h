/*
 * Copyright (c) 2022, Martin Falisse <mfalisse@outlook.com>
 * Copyright (c) 2025, Aliaksandr Kalenik <kalenik.aliaksandr@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/FlyString.h>
#include <AK/Vector.h>
#include <LibWeb/CSS/PercentageOr.h>
#include <LibWeb/Layout/AvailableSpace.h>

namespace Web::CSS {

class GridSize {
public:
    enum class Type {
        LengthPercentage,
        FlexibleLength,
        FitContent,
        MaxContent,
        MinContent,
    };

    GridSize(Type, LengthPercentage);
    GridSize(LengthPercentage);
    GridSize(Flex);
    GridSize(Type);
    ~GridSize();

    static GridSize make_auto();

    Type type() const { return m_type; }

    bool is_auto(Layout::AvailableSize const&) const;
    bool is_fixed(Layout::AvailableSize const&) const;
    bool is_flexible_length() const { return m_type == Type::FlexibleLength; }
    bool is_fit_content() const { return m_type == Type::FitContent; }
    bool is_max_content() const { return m_type == Type::MaxContent; }
    bool is_min_content() const { return m_type == Type::MinContent; }

    LengthPercentage const& length_percentage() const { return m_value.get<LengthPercentage>(); }
    double flex_factor() const { return m_value.get<Flex>().to_fr(); }

    // https://www.w3.org/TR/css-grid-2/#layout-algorithm
    // An intrinsic sizing function (min-content, max-content, auto, fit-content()).
    bool is_intrinsic(Layout::AvailableSize const&) const;

    bool is_definite() const
    {
        return type() == Type::LengthPercentage && !length_percentage().is_auto();
    }

    Size css_size() const;

    String to_string() const;
    bool operator==(GridSize const& other) const = default;

private:
    Type m_type;
    Variant<Empty, LengthPercentage, Flex> m_value;
};

class GridMinMax {
public:
    GridMinMax(CSS::GridSize min_grid_size, CSS::GridSize max_grid_size);

    GridSize const& min_grid_size() const& { return m_min_grid_size; }
    GridSize const& max_grid_size() const& { return m_max_grid_size; }

    String to_string() const;
    bool operator==(GridMinMax const& other) const = default;

private:
    GridSize m_min_grid_size;
    GridSize m_max_grid_size;
};

struct GridLineName {
    FlyString name;
    bool implicit { false };

    bool operator==(GridLineName const& other) const = default;
};

class GridLineNames {
public:
    void append(FlyString const& name) { m_names.append({ name }); }
    bool is_empty() const { return m_names.is_empty(); }
    auto const& names() const& { return m_names; }

    String to_string() const;

    bool operator==(GridLineNames const& other) const = default;

private:
    Vector<GridLineName> m_names;
};

class GridTrackSizeList {
public:
    static GridTrackSizeList make_none();

    Vector<CSS::ExplicitGridTrack> track_list() const;
    auto const& list() const { return m_list; }

    String to_string() const;
    bool operator==(GridTrackSizeList const& other) const;

    bool is_empty() const { return m_list.is_empty(); }

    void append(GridLineNames&&);
    void append(ExplicitGridTrack&&);

private:
    Vector<Variant<ExplicitGridTrack, GridLineNames>> m_list;
};

enum class GridRepeatType {
    AutoFit,
    AutoFill,
    Fixed,
};

struct GridRepeatParams {
    GridRepeatType type;
    size_t count { 0 };
};

class GridRepeat {
public:
    GridRepeat(GridTrackSizeList&&, GridRepeatParams const&);

    bool is_auto_fill() const { return m_type == GridRepeatType::AutoFill; }
    bool is_auto_fit() const { return m_type == GridRepeatType::AutoFit; }
    bool is_fixed() const { return m_type == GridRepeatType::Fixed; }
    size_t repeat_count() const
    {
        VERIFY(is_fixed());
        return m_repeat_count;
    }
    GridTrackSizeList const& grid_track_size_list() const& { return m_grid_track_size_list; }
    GridRepeatType type() const& { return m_type; }

    String to_string() const;
    bool operator==(GridRepeat const& other) const = default;

private:
    GridRepeatType m_type;
    GridTrackSizeList m_grid_track_size_list;
    size_t m_repeat_count { 0 };
};

class ExplicitGridTrack {
public:
    ExplicitGridTrack(Variant<GridRepeat, GridMinMax, GridSize>&& value);

    bool is_repeat() const { return m_value.has<GridRepeat>(); }
    GridRepeat const& repeat() const { return m_value.get<GridRepeat>(); }

    bool is_minmax() const { return m_value.has<GridMinMax>(); }
    GridMinMax const& minmax() const { return m_value.get<GridMinMax>(); }

    bool is_default() const { return m_value.has<GridSize>(); }
    GridSize const& grid_size() const { return m_value.get<GridSize>(); }

    String to_string() const;
    bool operator==(ExplicitGridTrack const& other) const = default;

private:
    Variant<GridRepeat, GridMinMax, GridSize> m_value;
};

}
