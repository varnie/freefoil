#ifndef ERRORS_H_INCLUDED
#define ERRORS_H_INCLUDED

namespace Freefoil {

    namespace Private {

        enum E_ERRORS {
            ident_expected_error,
            open_bracket_expected_error,
            closed_bracket_expected_error,
            data_expected_error,
            open_block_expected_error,
            closed_block_expected_error,
            stmt_end_expected_error,
            bool_expr_expected_error,
            bool_factor_expected_error,
            expr_expected_error,
            term_expected_error,
            factor_expected_error,
//TODO
        };
    }
}


#endif // ERRORS_H_INCLUDED
