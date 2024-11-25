/*
 * Copyright (c) 2021, Tim Flynn <trflynn89@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibUnicode/CurrencyCode.h>

namespace Unicode {

static auto const& ensure_currency_codes()
{
    // https://www.iso.org/iso-4217-currency-codes.html
    // https://www.six-group.com/dam/download/financial-information/data-center/iso-currrency/amendments/lists/list_one.xml
    static HashMap<StringView, CurrencyCode> currency_codes {
        { "AED"sv, { 2 } },
        { "AFN"sv, { 2 } },
        { "ALL"sv, { 2 } },
        { "AMD"sv, { 2 } },
        { "ANG"sv, { 2 } },
        { "AOA"sv, { 2 } },
        { "ARS"sv, { 2 } },
        { "AUD"sv, { 2 } },
        { "AWG"sv, { 2 } },
        { "AZN"sv, { 2 } },
        { "BAM"sv, { 2 } },
        { "BBD"sv, { 2 } },
        { "BDT"sv, { 2 } },
        { "BGN"sv, { 2 } },
        { "BHD"sv, { 3 } },
        { "BIF"sv, { 0 } },
        { "BMD"sv, { 2 } },
        { "BND"sv, { 2 } },
        { "BOB"sv, { 2 } },
        { "BOV"sv, { 2 } },
        { "BRL"sv, { 2 } },
        { "BSD"sv, { 2 } },
        { "BTN"sv, { 2 } },
        { "BWP"sv, { 2 } },
        { "BYN"sv, { 2 } },
        { "BZD"sv, { 2 } },
        { "CAD"sv, { 2 } },
        { "CDF"sv, { 2 } },
        { "CHE"sv, { 2 } },
        { "CHF"sv, { 2 } },
        { "CHW"sv, { 2 } },
        { "CLF"sv, { 4 } },
        { "CLP"sv, { 0 } },
        { "CNY"sv, { 2 } },
        { "COP"sv, { 2 } },
        { "COU"sv, { 2 } },
        { "CRC"sv, { 2 } },
        { "CUC"sv, { 2 } },
        { "CUP"sv, { 2 } },
        { "CVE"sv, { 2 } },
        { "CZK"sv, { 2 } },
        { "DJF"sv, { 0 } },
        { "DKK"sv, { 2 } },
        { "DOP"sv, { 2 } },
        { "DZD"sv, { 2 } },
        { "EGP"sv, { 2 } },
        { "ERN"sv, { 2 } },
        { "ETB"sv, { 2 } },
        { "EUR"sv, { 2 } },
        { "FJD"sv, { 2 } },
        { "FKP"sv, { 2 } },
        { "GBP"sv, { 2 } },
        { "GEL"sv, { 2 } },
        { "GHS"sv, { 2 } },
        { "GIP"sv, { 2 } },
        { "GMD"sv, { 2 } },
        { "GNF"sv, { 0 } },
        { "GTQ"sv, { 2 } },
        { "GYD"sv, { 2 } },
        { "HKD"sv, { 2 } },
        { "HNL"sv, { 2 } },
        { "HRK"sv, { 2 } },
        { "HTG"sv, { 2 } },
        { "HUF"sv, { 2 } },
        { "IDR"sv, { 2 } },
        { "ILS"sv, { 2 } },
        { "INR"sv, { 2 } },
        { "IQD"sv, { 3 } },
        { "IRR"sv, { 2 } },
        { "ISK"sv, { 0 } },
        { "JMD"sv, { 2 } },
        { "JOD"sv, { 3 } },
        { "JPY"sv, { 0 } },
        { "KES"sv, { 2 } },
        { "KGS"sv, { 2 } },
        { "KHR"sv, { 2 } },
        { "KMF"sv, { 0 } },
        { "KPW"sv, { 2 } },
        { "KRW"sv, { 0 } },
        { "KWD"sv, { 3 } },
        { "KYD"sv, { 2 } },
        { "KZT"sv, { 2 } },
        { "LAK"sv, { 2 } },
        { "LBP"sv, { 2 } },
        { "LKR"sv, { 2 } },
        { "LRD"sv, { 2 } },
        { "LSL"sv, { 2 } },
        { "LYD"sv, { 3 } },
        { "MAD"sv, { 2 } },
        { "MDL"sv, { 2 } },
        { "MGA"sv, { 2 } },
        { "MKD"sv, { 2 } },
        { "MMK"sv, { 2 } },
        { "MNT"sv, { 2 } },
        { "MOP"sv, { 2 } },
        { "MRU"sv, { 2 } },
        { "MUR"sv, { 2 } },
        { "MVR"sv, { 2 } },
        { "MWK"sv, { 2 } },
        { "MXN"sv, { 2 } },
        { "MXV"sv, { 2 } },
        { "MYR"sv, { 2 } },
        { "MZN"sv, { 2 } },
        { "NAD"sv, { 2 } },
        { "NGN"sv, { 2 } },
        { "NIO"sv, { 2 } },
        { "NOK"sv, { 2 } },
        { "NPR"sv, { 2 } },
        { "NZD"sv, { 2 } },
        { "OMR"sv, { 3 } },
        { "PAB"sv, { 2 } },
        { "PEN"sv, { 2 } },
        { "PGK"sv, { 2 } },
        { "PHP"sv, { 2 } },
        { "PKR"sv, { 2 } },
        { "PLN"sv, { 2 } },
        { "PYG"sv, { 0 } },
        { "QAR"sv, { 2 } },
        { "RON"sv, { 2 } },
        { "RSD"sv, { 2 } },
        { "RUB"sv, { 2 } },
        { "RWF"sv, { 0 } },
        { "SAR"sv, { 2 } },
        { "SBD"sv, { 2 } },
        { "SCR"sv, { 2 } },
        { "SDG"sv, { 2 } },
        { "SEK"sv, { 2 } },
        { "SGD"sv, { 2 } },
        { "SHP"sv, { 2 } },
        { "SLL"sv, { 2 } },
        { "SOS"sv, { 2 } },
        { "SRD"sv, { 2 } },
        { "SSP"sv, { 2 } },
        { "STN"sv, { 2 } },
        { "SVC"sv, { 2 } },
        { "SYP"sv, { 2 } },
        { "SZL"sv, { 2 } },
        { "THB"sv, { 2 } },
        { "TJS"sv, { 2 } },
        { "TMT"sv, { 2 } },
        { "TND"sv, { 3 } },
        { "TOP"sv, { 2 } },
        { "TRY"sv, { 2 } },
        { "TTD"sv, { 2 } },
        { "TWD"sv, { 2 } },
        { "TZS"sv, { 2 } },
        { "UAH"sv, { 2 } },
        { "UGX"sv, { 0 } },
        { "USD"sv, { 2 } },
        { "USN"sv, { 2 } },
        { "UYI"sv, { 0 } },
        { "UYU"sv, { 2 } },
        { "UYW"sv, { 4 } },
        { "UZS"sv, { 2 } },
        { "VES"sv, { 2 } },
        { "VND"sv, { 0 } },
        { "VUV"sv, { 0 } },
        { "WST"sv, { 2 } },
        { "XAF"sv, { 0 } },
        { "XAG"sv, { {} } },
        { "XAU"sv, { {} } },
        { "XBA"sv, { {} } },
        { "XBB"sv, { {} } },
        { "XBC"sv, { {} } },
        { "XBD"sv, { {} } },
        { "XCD"sv, { 2 } },
        { "XDR"sv, { {} } },
        { "XOF"sv, { 0 } },
        { "XPD"sv, { {} } },
        { "XPF"sv, { 0 } },
        { "XPT"sv, { {} } },
        { "XSU"sv, { {} } },
        { "XTS"sv, { {} } },
        { "XUA"sv, { {} } },
        { "XXX"sv, { {} } },
        { "YER"sv, { 2 } },
        { "ZAR"sv, { 2 } },
        { "ZMW"sv, { 2 } },
        { "ZWL"sv, { 2 } },
    };

    return currency_codes;
}

Optional<CurrencyCode const&> get_currency_code(StringView currency)
{
    static auto const& currency_codes = ensure_currency_codes();
    return currency_codes.get(currency);
}

}
