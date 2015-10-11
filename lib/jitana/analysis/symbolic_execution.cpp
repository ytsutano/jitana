#include "jitana/analysis/symbolic_execution.hpp"

#include <fstream>

#include <boost/range/adaptor/reversed.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <yices.h>

using namespace jitana;

#define PRINT_TREE 1
#ifdef PRINT_TREE
namespace {
    std::ofstream ofs_tree("output/tree.dot");
}
#endif

namespace {
    struct sym_expr {
        char type_desc = '?';
        term_t yices_term = NULL_TERM;

        sym_expr(char type_desc = '?') : type_desc(type_desc)
        {
        }

        virtual ~sym_expr()
        {
        }

        virtual void print(std::ostream& os) const
        {
            os << type_desc;
        }

        friend std::ostream& operator<<(std::ostream& os, const sym_expr& x)
        {
            x.print(os);
            return os;
        }
    };

    enum sym_binary_op {
        sym_binary_op_add,
        sym_binary_op_sub,
        sym_binary_op_mul,
        sym_binary_op_div,
        sym_binary_op_rem,
        sym_binary_op_and,
        sym_binary_op_or,
        sym_binary_op_xor,
        sym_binary_op_shl,
        sym_binary_op_shr,
        sym_binary_op_ushr,
        sym_binary_op_rsub,
        sym_binary_op_eq,
        sym_binary_op_ne,
        sym_binary_op_lt,
        sym_binary_op_ge,
        sym_binary_op_gt,
        sym_binary_op_le
    };

    inline std::ostream& operator<<(std::ostream& os, const sym_binary_op& x)
    {
        const char* strs[]
                = {"+",  "-",   "*",      "/",  "%%", "&", "|",  "^", "<<",
                   ">>", ">>>", "<rsub>", "==", "!=", "<", ">=", ">", "<="};
        return os << strs[x];
    }

    enum sym_unary_op { sym_unary_op_neg, sym_unary_op_not, sym_unary_op_cast };

    inline std::ostream& operator<<(std::ostream& os, const sym_unary_op& x)
    {
        const char* strs[] = {"~", "!", "<cast>"};
        return os << strs[x];
    }

    struct sym_unary_op_expr : sym_expr {
        sym_unary_op op;
        std::shared_ptr<const sym_expr> expr;

        sym_unary_op_expr(char type_desc, sym_unary_op op,
                          std::shared_ptr<const sym_expr> expr)
                : sym_expr(type_desc), op(op), expr(expr)
        {
            switch (op) {
            case sym_unary_op_neg:
                yices_term = yices_neg(expr->yices_term);
                break;
            case sym_unary_op_not:
                yices_term = yices_not(expr->yices_term);
                break;
            case sym_unary_op_cast:
                yices_term = expr->yices_term;
                break;
            }
        }

        void print(std::ostream& os) const override
        {
            os << op << "(" << *expr << "):" << type_desc;
        }
    };

    struct sym_binary_op_expr : sym_expr {
        sym_binary_op op;
        std::shared_ptr<const sym_expr> lhs;
        std::shared_ptr<const sym_expr> rhs;

        sym_binary_op_expr(char type_desc, sym_binary_op op,
                           std::shared_ptr<const sym_expr> lhs,
                           std::shared_ptr<const sym_expr> rhs)
                : sym_expr(type_desc), op(op), lhs(lhs), rhs(rhs)
        {
            term_t yices_lhs = lhs->yices_term;
            term_t yices_rhs = rhs->yices_term;

            switch (op) {
            case sym_binary_op_add:
                yices_term = yices_bvadd(yices_lhs, yices_rhs);
                break;
            case sym_binary_op_sub:
                yices_term = yices_bvsub(yices_lhs, yices_rhs);
                break;
            case sym_binary_op_mul:
                yices_term = yices_bvmul(yices_lhs, yices_rhs);
                break;
            case sym_binary_op_div:
                break;
            case sym_binary_op_rem:
                break;
            case sym_binary_op_and:
                break;
            case sym_binary_op_or:
                break;
            case sym_binary_op_xor:
                break;
            case sym_binary_op_shl:
                break;
            case sym_binary_op_shr:
                break;
            case sym_binary_op_ushr:
                break;
            case sym_binary_op_rsub:
                break;
            case sym_binary_op_eq:
                yices_term = yices_bveq_atom(yices_lhs, yices_rhs);
                break;
            case sym_binary_op_ne:
                yices_term = yices_bvneq_atom(yices_lhs, yices_rhs);
                break;
            case sym_binary_op_lt:
                yices_term = yices_bvslt_atom(yices_lhs, yices_rhs);
                break;
            case sym_binary_op_ge:
                yices_term = yices_bvsge_atom(yices_lhs, yices_rhs);
                break;
            case sym_binary_op_gt:
                yices_term = yices_bvsgt_atom(yices_lhs, yices_rhs);
                break;
            case sym_binary_op_le:
                yices_term = yices_bvsle_atom(yices_lhs, yices_rhs);
                break;
            }
        }

        void print(std::ostream& os) const override
        {
            os << "(" << *lhs << ")" << op << "(" << *rhs << "):" << type_desc;
        }
    };

    struct sym_obj_expr : sym_expr {
        dex_type_hdl type_hdl;
        uint32_t val;

        sym_obj_expr(char type_desc, dex_type_hdl type_hdl, int32_t val)
                : sym_expr(type_desc), type_hdl(type_hdl), val(val)
        {
        }

        void print(std::ostream& os) const override
        {
            os << "@" << val << ":" << type_hdl;
        }
    };

    struct sym_val_expr : sym_expr {
        int32_t val;

        sym_val_expr(char type_desc, int32_t val)
                : sym_expr(type_desc), val(val)
        {
            yices_term = yices_bvconst_int32(32, val);
        }

        void print(std::ostream& os) const override
        {
            os << val << ":" << type_desc;
        }
    };

    struct sym_string_expr : sym_expr {
        std::string str;

        sym_string_expr(char type_desc, std::string str)
                : sym_expr(type_desc), str(str)
        {
        }

        void print(std::ostream& os) const override
        {
            os << str << ":" << type_desc;
        }
    };

    struct sym_array_expr : sym_expr {
        std::vector<std::shared_ptr<const sym_expr>> array;
        std::shared_ptr<const sym_expr> size;
        dex_type_hdl elem_type_hdl;

        sym_array_expr(char type_desc, std::shared_ptr<const sym_expr> size,
                       dex_type_hdl elem_type_hdl)
                : sym_expr(type_desc), size(size), elem_type_hdl(elem_type_hdl)
        {
        }

        void print(std::ostream& os) const override
        {
            os << "[";
            for (const auto& e : array) {
                os << " (" << e << ")";
            }
            os << " ]:" << type_desc;
        }
    };

    struct sym_class_expr : sym_expr {
        dex_type_hdl hdl;

        sym_class_expr(char type_desc, dex_type_hdl hdl)
                : sym_expr(type_desc), hdl(hdl)
        {
        }

        void print(std::ostream& os) const override
        {
            os << hdl << ":" << type_desc;
        }
    };

    struct sym_wide_val_expr : sym_expr {
        int64_t val;

        sym_wide_val_expr(char type_desc, int64_t val)
                : sym_expr(type_desc), val(val)
        {
        }

        void print(std::ostream& os) const override
        {
            os << val << ":" << type_desc;
        }
    };

    struct sym_param_reg_expr : sym_expr {
        register_idx idx;

        sym_param_reg_expr(char type_desc, register_idx idx)
                : sym_expr(type_desc), idx(idx)
        {
            switch (type_desc) {
            case 'Z':
                yices_term = yices_new_uninterpreted_term(yices_bool_type());
                break;
            case 'B':
                yices_term = yices_new_uninterpreted_term(yices_bv_type(8));
                break;
            case 'S':
            case 'C':
                yices_term = yices_new_uninterpreted_term(yices_bv_type(16));
                break;
            case 'I':
            case 'F':
                yices_term = yices_new_uninterpreted_term(yices_bv_type(32));
                break;
            case 'J':
            case 'D':
                yices_term = yices_new_uninterpreted_term(yices_bv_type(64));
                break;
            }

            std::stringstream ss;
            ss << idx;
            yices_set_term_name(yices_term, ss.str().c_str());
        }

        void print(std::ostream& os) const override
        {
            os << idx << ":" << type_desc;
        }
    };

    struct sym_instance_field_expr : sym_expr {
        dex_field_hdl field;
        std::shared_ptr<const sym_expr> obj;

        sym_instance_field_expr(char type_desc, dex_field_hdl field,
                                std::shared_ptr<const sym_expr> obj)
                : sym_expr(type_desc), field(field), obj(obj)
        {
        }
    };

    struct sym_static_field_expr : sym_expr {
        dex_field_hdl field;

        sym_static_field_expr(char type_desc, dex_field_hdl field)
                : sym_expr(type_desc), field(field)
        {
            switch (type_desc) {
            case 'Z':
                yices_term = yices_new_uninterpreted_term(yices_bool_type());
                break;
            case 'B':
                yices_term = yices_new_uninterpreted_term(yices_bv_type(8));
                break;
            case 'S':
            case 'C':
                yices_term = yices_new_uninterpreted_term(yices_bv_type(16));
                break;
            case 'I':
            case 'F':
            case 'L':
                yices_term = yices_new_uninterpreted_term(yices_bv_type(32));
                break;
            case 'J':
            case 'D':
                yices_term = yices_new_uninterpreted_term(yices_bv_type(64));
                break;
            default:
                std::cerr << type_desc << "\n";
                throw std::runtime_error("unknown type");
            }

            std::stringstream ss;
            ss << field;
            yices_set_term_name(yices_term, ss.str().c_str());
        }
    };
}

namespace {
    struct sym_path_condition {
        std::shared_ptr<const sym_expr> expr;

        sym_path_condition(std::shared_ptr<const sym_expr> expr) : expr(expr)
        {
        }
    };

    struct sym_post_state {
        std::shared_ptr<const sym_expr> expr;

        sym_post_state(std::shared_ptr<const sym_expr> expr) : expr(expr)
        {
        }
    };

    struct sym_path_summary {
        std::vector<sym_path_condition> path_conditions;
        std::vector<sym_post_state> post_states;
        std::vector<insn_vertex_descriptor> log;

        friend std::ostream& operator<<(std::ostream& os,
                                        const sym_path_summary& x)
        {
            return os;
        }
    };
}

namespace {
    class uninitialized_read_error : public std::runtime_error {
        using runtime_error::runtime_error;
    };

    struct sym_exec_bb_state {
#ifdef PRINT_TREE
        mutable int state_id = 0;
#endif
        insn_control_flow_branch_cond branch_cond;
        std::shared_ptr<const sym_expr> branch_expr;
        std::shared_ptr<const sym_expr> path_cond;

        const sym_exec_bb_state* prev_bb_state;

        std::unordered_map<register_idx, std::shared_ptr<const sym_expr>> regs;
        std::unordered_map<dex_field_hdl, std::shared_ptr<const sym_expr>>
                static_fields;

        uint32_t n_objs = 0;
        std::vector<insn_vertex_descriptor> log;

        uint32_t make_new_object_id()
        {
            return n_objs++;
        }

        void write_reg(register_idx idx, std::shared_ptr<const sym_expr> expr)
        {
            if (idx == register_idx::idx_unknown) {
                // std::cerr << idx << "\n";
                throw std::runtime_error("write_reg: invalid register index");
            }

            regs[idx] = expr;
        }

        const std::shared_ptr<const sym_expr>& read_reg(register_idx idx) const
        {
            if (idx == register_idx::idx_unknown) {
                // std::cerr << idx << "\n";
                throw std::runtime_error("read_reg: invalid register index");
            }

            auto it = regs.find(idx);
            if (it != regs.end()) {
                return it->second;
            }

            if (prev_bb_state == nullptr) {
                // std::cerr << idx << "\n";
                throw uninitialized_read_error(
                        "read_reg: uninitialized register");
            }

            return prev_bb_state->read_reg(idx);
        }

        void write_static_field(const dex_field_hdl& hdl,
                                std::shared_ptr<const sym_expr> expr)
        {
            static_fields[hdl] = expr;
        }

        const std::shared_ptr<const sym_expr>&
        read_static_field(const dex_field_hdl& hdl) const
        {
            auto it = static_fields.find(hdl);
            if (it != static_fields.end()) {
                return it->second;
            }

            if (prev_bb_state == nullptr) {
                throw uninitialized_read_error("invalid field handle");
            }

            return prev_bb_state->read_static_field(hdl);
        }
    };
}

namespace {
    class sym_update_state : public boost::static_visitor<void> {
    public:
        sym_update_state(sym_exec_bb_state& bb_state, virtual_machine& vm,
                         const method_vertex_property& mvprop)
                : bb_state_(bb_state), vm_(vm), mvprop_(mvprop)
        {
        }

        void operator()(const insn_entry& x)
        {
            const auto& params = mvprop_.params;
            auto current_reg = x.regs[0];

            // Non-static method has a this pointer as the first argument.
            if (!(mvprop_.access_flags & acc_static)) {
                auto expr = std::make_shared<sym_param_reg_expr>('L',
                                                                 current_reg);
                bb_state_.write_reg(current_reg, expr);
                ++current_reg;
            }

            // Create a symbolic variable for each input parameter registers.
            for (const auto& p : params) {
                char desc = p.descriptor[0];

                auto expr = std::make_shared<sym_param_reg_expr>(desc,
                                                                 current_reg);
                bb_state_.write_reg(current_reg, expr);
                ++current_reg;

                // Ignore next register if the parameter is a wide type.
                if (desc == 'J' || desc == 'D') {
                    ++current_reg;
                }
            }
        }

        void operator()(const insn_nop& x)
        {
        }

        void operator()(const insn_move& x)
        {
            bb_state_.write_reg(x.regs[0], bb_state_.read_reg(x.regs[1]));
        }

        void operator()(const insn_return& x)
        {
            switch (x.op) {
            case opcode::op_return_void:
            case opcode::op_return_void_barrier:
                break;
            case opcode::op_return:
            case opcode::op_return_wide:
            case opcode::op_return_object:
                bb_state_.write_reg(register_idx::idx_result,
                                    bb_state_.read_reg(x.regs[0]));
                break;
            default:
                break;
            }
        }

        void operator()(const insn_const& x)
        {
            auto expr = std::make_shared<sym_val_expr>('I', x.const_val);
            bb_state_.write_reg(x.regs[0], expr);
        }

        void operator()(const insn_const_wide& x)
        {
            auto expr = std::make_shared<sym_wide_val_expr>('J', x.const_val);
            bb_state_.write_reg(x.regs[0], expr);
        }

        void operator()(const insn_const_string& x)
        {
            auto expr = std::make_shared<sym_string_expr>('s', x.const_val);
            bb_state_.write_reg(x.regs[0], expr);
        }

        void operator()(const insn_const_class& x)
        {
            auto expr = std::make_shared<sym_class_expr>('c', x.const_val);
            bb_state_.write_reg(x.regs[0], expr);
        }

        void operator()(const insn_monitor_enter& x)
        {
        }

        void operator()(const insn_monitor_exit& x)
        {
        }

        // void operator()(const insn_check_cast& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_instance_of& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_array_length& x)
        // {
        //     // TODO: Implmement.
        // }

        void operator()(const insn_new_instance& x)
        {
            auto expr = std::make_shared<sym_obj_expr>(
                    'L', x.const_val, bb_state_.make_new_object_id());
            bb_state_.write_reg(x.regs[0], expr);
        }

        void operator()(const insn_new_array& x)
        {
            auto expr = std::make_shared<sym_array_expr>(
                    '[', bb_state_.read_reg(x.regs[1]), x.const_val);
            bb_state_.write_reg(x.regs[0], expr);
        }

        // void operator()(const insn_filled_new_array& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_fill_array_data& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_throw& x)
        // {
        //     // TODO: Implmement.
        // }

        void operator()(const insn_goto& x)
        {
        }

        // void operator()(const insn_switch& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_cmp& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_if& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_if_z& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_aget& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_aput& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_iget& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        // void operator()(const insn_iput& x)
        // {
        //     // TODO: Implmement.
        // }

        void operator()(const insn_sget& x)
        {
            const auto& reg = x.regs[0];
            const auto& hdl = x.const_val;

            try {
                bb_state_.write_reg(reg, bb_state_.read_static_field(hdl));
            }
            catch (uninitialized_read_error& e) {
                // Create a field. FIXME: descriptor is wrong.
                const auto& descriptor = vm_.jvm_hdl(hdl).type_hdl.descriptor;
                auto expr = std::make_shared<sym_static_field_expr>(
                        descriptor[0], hdl);
                bb_state_.write_static_field(hdl, expr);
                bb_state_.write_reg(reg, expr);
            }
        }

        void operator()(const insn_sput& x)
        {
            const auto& reg = x.regs[0];
            const auto& hdl = x.const_val;

            bb_state_.write_static_field(hdl, bb_state_.read_reg(reg));
        }

        void operator()(const insn_invoke& x)
        {
            // TODO: Implmement.
            // FIXME: for now return zero.
            auto expr = std::make_shared<sym_val_expr>('I', 0);
            bb_state_.write_reg(register_idx::idx_result, expr);
        }
        //
        // void operator()(const insn_invoke_quick& x)
        // {
        //     // TODO: Implmement.
        // }
        //
        void operator()(const insn_unary_arith_op& x)
        {
            sym_unary_op sop;
            switch (op(x)) {
            case opcode::op_neg_double:
            case opcode::op_neg_float:
            case opcode::op_neg_int:
            case opcode::op_neg_long:
                sop = sym_unary_op_neg;
                break;
            case opcode::op_not_int:
            case opcode::op_not_long:
                sop = sym_unary_op_not;
                break;
            case opcode::op_double_to_float:
            case opcode::op_double_to_int:
            case opcode::op_double_to_long:
            case opcode::op_float_to_double:
            case opcode::op_float_to_int:
            case opcode::op_float_to_long:
            case opcode::op_int_to_byte:
            case opcode::op_int_to_char:
            case opcode::op_int_to_double:
            case opcode::op_int_to_float:
            case opcode::op_int_to_long:
            case opcode::op_int_to_short:
            case opcode::op_long_to_double:
            case opcode::op_long_to_float:
            case opcode::op_long_to_int:
                sop = sym_unary_op_cast;
                break;
            default:
                throw std::runtime_error("invalid instruction");
            }

            char desc;
            switch (op(x)) {
            case opcode::op_neg_int:
            case opcode::op_not_int:
            case opcode::op_float_to_int:
            case opcode::op_long_to_int:
            case opcode::op_double_to_int:
                desc = 'I';
                break;
            case opcode::op_neg_long:
            case opcode::op_not_long:
            case opcode::op_int_to_long:
            case opcode::op_double_to_long:
            case opcode::op_float_to_long:
                desc = 'J';
                break;
            case opcode::op_neg_float:
            case opcode::op_int_to_float:
            case opcode::op_long_to_float:
            case opcode::op_double_to_float:
                desc = 'F';
                break;
            case opcode::op_neg_double:
            case opcode::op_int_to_double:
            case opcode::op_long_to_double:
            case opcode::op_float_to_double:
                desc = 'D';
                break;
            case opcode::op_int_to_byte:
                desc = 'B';
                break;
            case opcode::op_int_to_char:
                desc = 'C';
                break;
            case opcode::op_int_to_short:
                desc = 'S';
                break;
            default:
                throw std::runtime_error("invalid instruction");
            }

            auto& arg = bb_state_.read_reg(x.regs[1]);

            auto expr = std::make_shared<sym_unary_op_expr>(desc, sop, arg);
            bb_state_.write_reg(x.regs[0], expr);
        }

        void operator()(const insn_binary_arith_op& x)
        {
            sym_binary_op sop;
            switch (op(x)) {
            case opcode::op_add_double:
            case opcode::op_add_float:
            case opcode::op_add_int:
            case opcode::op_add_long:
                sop = sym_binary_op_add;
                break;
            case opcode::op_and_int:
            case opcode::op_and_long:
                sop = sym_binary_op_and;
                break;
            case opcode::op_div_double:
            case opcode::op_div_float:
            case opcode::op_div_int:
            case opcode::op_div_long:
                sop = sym_binary_op_div;
                break;
            case opcode::op_mul_double:
            case opcode::op_mul_float:
            case opcode::op_mul_int:
            case opcode::op_mul_long:
                sop = sym_binary_op_mul;
                break;
            case opcode::op_or_int:
            case opcode::op_or_long:
                sop = sym_binary_op_or;
                break;
            case opcode::op_rem_double:
            case opcode::op_rem_float:
            case opcode::op_rem_int:
            case opcode::op_rem_long:
                sop = sym_binary_op_rem;
                break;
            case opcode::op_shl_int:
            case opcode::op_shl_long:
                sop = sym_binary_op_shl;
                break;
            case opcode::op_shr_int:
            case opcode::op_shr_long:
                sop = sym_binary_op_shr;
                break;
            case opcode::op_sub_double:
            case opcode::op_sub_float:
            case opcode::op_sub_int:
            case opcode::op_sub_long:
                sop = sym_binary_op_sub;
                break;
            case opcode::op_ushr_int:
            case opcode::op_ushr_long:
                sop = sym_binary_op_ushr;
                break;
            case opcode::op_xor_int:
            case opcode::op_xor_long:
                sop = sym_binary_op_xor;
                break;
            default:
                throw std::runtime_error("invalid instruction");
            }

            char desc;
            switch (op(x)) {
            case opcode::op_add_int:
            case opcode::op_sub_int:
            case opcode::op_mul_int:
            case opcode::op_div_int:
            case opcode::op_rem_int:
            case opcode::op_and_int:
            case opcode::op_or_int:
            case opcode::op_xor_int:
            case opcode::op_shl_int:
            case opcode::op_shr_int:
            case opcode::op_ushr_int:
                desc = 'I';
                break;
            case opcode::op_add_long:
            case opcode::op_sub_long:
            case opcode::op_mul_long:
            case opcode::op_div_long:
            case opcode::op_rem_long:
            case opcode::op_and_long:
            case opcode::op_or_long:
            case opcode::op_xor_long:
            case opcode::op_shl_long:
            case opcode::op_shr_long:
            case opcode::op_ushr_long:
                desc = 'J';
                break;
            case opcode::op_add_float:
            case opcode::op_sub_float:
            case opcode::op_mul_float:
            case opcode::op_div_float:
            case opcode::op_rem_float:
                desc = 'F';
                break;
            case opcode::op_add_double:
            case opcode::op_sub_double:
            case opcode::op_mul_double:
            case opcode::op_div_double:
            case opcode::op_rem_double:
                desc = 'D';
                break;
            default:
                throw std::runtime_error("invalid instruction");
            }

            auto& lhs = bb_state_.read_reg(x.regs[1]);
            auto& rhs = bb_state_.read_reg(x.regs[2]);

            auto expr
                    = std::make_shared<sym_binary_op_expr>(desc, sop, lhs, rhs);
            bb_state_.write_reg(x.regs[0], expr);
        }

        void operator()(const insn_aug_assign_op& x)
        {
            sym_binary_op sop;
            switch (op(x)) {
            case opcode::op_add_double_2addr:
            case opcode::op_add_float_2addr:
            case opcode::op_add_int_2addr:
            case opcode::op_add_long_2addr:
            case opcode::op_and_int_2addr:
            case opcode::op_and_long_2addr:
                sop = sym_binary_op_and;
                break;
            case opcode::op_div_double_2addr:
            case opcode::op_div_float_2addr:
            case opcode::op_div_int_2addr:
            case opcode::op_div_long_2addr:
                sop = sym_binary_op_div;
                break;
            case opcode::op_mul_double_2addr:
            case opcode::op_mul_float_2addr:
            case opcode::op_mul_int_2addr:
            case opcode::op_mul_long_2addr:
                sop = sym_binary_op_mul;
                break;
            case opcode::op_or_int_2addr:
            case opcode::op_or_long_2addr:
                sop = sym_binary_op_or;
                break;
            case opcode::op_rem_double_2addr:
            case opcode::op_rem_float_2addr:
            case opcode::op_rem_int_2addr:
            case opcode::op_rem_long_2addr:
                sop = sym_binary_op_rem;
                break;
            case opcode::op_shl_int_2addr:
            case opcode::op_shl_long_2addr:
                sop = sym_binary_op_shl;
                break;
            case opcode::op_shr_int_2addr:
            case opcode::op_shr_long_2addr:
                sop = sym_binary_op_shr;
                break;
            case opcode::op_sub_double_2addr:
            case opcode::op_sub_float_2addr:
            case opcode::op_sub_int_2addr:
            case opcode::op_sub_long_2addr:
                sop = sym_binary_op_sub;
                break;
            case opcode::op_ushr_int_2addr:
            case opcode::op_ushr_long_2addr:
                sop = sym_binary_op_ushr;
                break;
            case opcode::op_xor_int_2addr:
            case opcode::op_xor_long_2addr:
                sop = sym_binary_op_xor;
                break;
            default:
                throw std::runtime_error("invalid instruction");
            }

            char desc;
            switch (op(x)) {
            case opcode::op_add_int_2addr:
            case opcode::op_sub_int_2addr:
            case opcode::op_mul_int_2addr:
            case opcode::op_div_int_2addr:
            case opcode::op_rem_int_2addr:
            case opcode::op_and_int_2addr:
            case opcode::op_or_int_2addr:
            case opcode::op_xor_int_2addr:
            case opcode::op_shl_int_2addr:
            case opcode::op_shr_int_2addr:
            case opcode::op_ushr_int_2addr:
                desc = 'I';
                break;
            case opcode::op_add_long_2addr:
            case opcode::op_sub_long_2addr:
            case opcode::op_mul_long_2addr:
            case opcode::op_div_long_2addr:
            case opcode::op_rem_long_2addr:
            case opcode::op_and_long_2addr:
            case opcode::op_or_long_2addr:
            case opcode::op_xor_long_2addr:
            case opcode::op_shl_long_2addr:
            case opcode::op_shr_long_2addr:
            case opcode::op_ushr_long_2addr:
                desc = 'J';
                break;
            case opcode::op_add_float_2addr:
            case opcode::op_sub_float_2addr:
            case opcode::op_mul_float_2addr:
            case opcode::op_div_float_2addr:
            case opcode::op_rem_float_2addr:
                desc = 'F';
                break;
            case opcode::op_add_double_2addr:
            case opcode::op_sub_double_2addr:
            case opcode::op_mul_double_2addr:
            case opcode::op_div_double_2addr:
            case opcode::op_rem_double_2addr:
                desc = 'D';
                break;
            default:
                throw std::runtime_error("invalid instruction");
            }

            auto& lhs = bb_state_.read_reg(x.regs[0]);
            auto& rhs = bb_state_.read_reg(x.regs[1]);

            auto expr
                    = std::make_shared<sym_binary_op_expr>(desc, sop, lhs, rhs);
            bb_state_.write_reg(x.regs[0], expr);
        }

        void operator()(const insn_const_arith_op& x)
        {
            sym_binary_op sop;
            switch (op(x)) {
            case opcode::op_add_int_lit16:
            case opcode::op_add_int_lit8:
                sop = sym_binary_op_add;
                break;
            case opcode::op_rsub_int:
            case opcode::op_rsub_int_lit8:
                sop = sym_binary_op_rsub;
                break;
            case opcode::op_mul_int_lit16:
            case opcode::op_mul_int_lit8:
                sop = sym_binary_op_mul;
                break;
            case opcode::op_div_int_lit16:
            case opcode::op_div_int_lit8:
                sop = sym_binary_op_div;
                break;
            case opcode::op_rem_int_lit16:
            case opcode::op_rem_int_lit8:
                sop = sym_binary_op_rem;
                break;
            case opcode::op_and_int_lit16:
            case opcode::op_and_int_lit8:
                sop = sym_binary_op_and;
                break;
            case opcode::op_or_int_lit16:
            case opcode::op_or_int_lit8:
                sop = sym_binary_op_or;
                break;
            case opcode::op_xor_int_lit16:
            case opcode::op_xor_int_lit8:
                sop = sym_binary_op_xor;
                break;
            case opcode::op_shl_int_lit8:
                sop = sym_binary_op_shl;
                break;
            case opcode::op_shr_int_lit8:
                sop = sym_binary_op_shr;
                break;
            case opcode::op_ushr_int_lit8:
                sop = sym_binary_op_ushr;
                break;
            default:
                throw std::runtime_error("invalid instruction");
            }

            auto& lhs = bb_state_.read_reg(x.regs[1]);
            auto rhs = std::make_shared<sym_val_expr>('I', x.const_val);

            auto expr
                    = std::make_shared<sym_binary_op_expr>('I', sop, lhs, rhs);
            bb_state_.write_reg(x.regs[0], expr);
        }

        template <typename T>
        void operator()(const T& x)
        {
            // std::cout << "-----------------------------------------\n";
            // std::cout << x << "\n";
            // std::cout << "-----------------------------------------\n";
            // std::cout << "*** not implemented ***\n";
            // std::cout << std::endl;
        }

    private:
        sym_exec_bb_state& bb_state_;
        virtual_machine& vm_;
        const method_vertex_property& mvprop_;
    };

    class sym_update_pc : public boost::static_visitor<bool> {
    public:
        sym_update_pc(sym_exec_bb_state& bb_state) : bb_state_(bb_state)
        {
        }

        bool operator()(const insn_if& x)
        {
            sym_binary_op sop;
            switch (op(x)) {
            case opcode::op_if_eq:
                sop = sym_binary_op_eq;
                break;
            case opcode::op_if_ne:
                sop = sym_binary_op_ne;
                break;
            case opcode::op_if_lt:
                sop = sym_binary_op_lt;
                break;
            case opcode::op_if_ge:
                sop = sym_binary_op_ge;
                break;
            case opcode::op_if_gt:
                sop = sym_binary_op_gt;
                break;
            case opcode::op_if_le:
                sop = sym_binary_op_le;
                break;
            default:
                throw std::runtime_error("invalid instruction");
                break;
            }

            const auto& lhs = bb_state_.read_reg(x.regs[0]);
            const auto& rhs = bb_state_.read_reg(x.regs[1]);

            bb_state_.branch_expr
                    = std::make_shared<sym_binary_op_expr>('Z', sop, lhs, rhs);

            return true;
        }

        bool operator()(const insn_if_z& x)
        {
            sym_binary_op sop;
            switch (op(x)) {
            case opcode::op_if_eqz:
                sop = sym_binary_op_eq;
                break;
            case opcode::op_if_nez:
                sop = sym_binary_op_ne;
                break;
            case opcode::op_if_ltz:
                sop = sym_binary_op_lt;
                break;
            case opcode::op_if_gez:
                sop = sym_binary_op_ge;
                break;
            case opcode::op_if_gtz:
                sop = sym_binary_op_gt;
                break;
            case opcode::op_if_lez:
                sop = sym_binary_op_le;
                break;
            default:
                throw std::runtime_error("invalid instruction");
                break;
            }

            const auto& lhs = bb_state_.read_reg(x.regs[0]);
            const auto& rhs = std::make_shared<sym_val_expr>('I', 0);

            bb_state_.branch_expr
                    = std::make_shared<sym_binary_op_expr>('Z', sop, lhs, rhs);

            return true;
        }

        bool operator()(const insn_switch& x)
        {
            switch (op(x)) {
            case opcode::op_packed_switch:
            case opcode::op_sparse_switch:
                break;
            default:
                throw std::runtime_error("invalid instruction");
                break;
            }

            bb_state_.branch_expr = bb_state_.read_reg(x.regs[0]);

            return true;
        }

        template <typename T>
        bool operator()(const T& x)
        {
            return false;
        }

    private:
        sym_exec_bb_state& bb_state_;
    };
}

namespace {
    bool check_satisfiability(const sym_exec_bb_state& state)
    {
        context_t* ctx = yices_new_context(nullptr);

        for (const auto* s = &state; s != nullptr; s = s->prev_bb_state) {
            if (s->path_cond == nullptr) {
                // std::cout << "state.path_cond=nullptr\n";
                continue;
            }

            term_t f = s->path_cond->yices_term;
            // std::cout << "state.path_cond=" << *s->path_cond << "\n";
            // print_yices_term(f);
            if (f != NULL_TERM) {
                auto code = yices_assert_formula(ctx, f);
                if (code < 0) {
                    yices_print_error(stderr);
                    std::cerr << "ERROR!\n";
                }
            }
        }

        bool satisfiable = (yices_check_context(ctx, nullptr) == STATUS_SAT);

#ifdef PRINT_TREE
        {
            if (satisfiable) {
                if (model_t* model = yices_get_model(ctx, true)) {
                    char* cs = yices_model_to_string(model, 80, 4, 0);
                    std::string s = cs;
                    boost::replace_all(s, "\n", "\\n");
                    ofs_tree << s << "|";
                    yices_free_string(cs);
                    yices_free_model(model);
                }
            }
        }
#endif

        yices_free_context(ctx);
        return satisfiable;
    }

    void sym_make_summary(const sym_exec_bb_state& state)
    {
#if 1

        std::vector<const sym_exec_bb_state*> states;
        for (const auto* s = &state; s != nullptr; s = s->prev_bb_state) {
            states.push_back(s);
        }

#if 1
        std::cout << "log=";
        for (const auto* s : boost::adaptors::reverse(states)) {
            std::cout << "\t";
            std::cout << std::boolalpha << s->branch_cond;
            std::cout << "[";
            for (const auto& iv : s->log) {
                std::cout << " " << iv;
            }
            std::cout << " ]\n";
        }
        std::cout << "\n";
#endif
#endif
    }
}

namespace {
    template <typename CFG>
    bool run_basic_block(virtual_machine& vm,
                         const method_vertex_descriptor& mv, const CFG& cfg,
                         insn_vertex_descriptor head_iv,
                         insn_control_flow_branch_cond branch_cond,
                         const sym_exec_bb_state* prev_state, int bb_len_limit)
    {
        if (bb_len_limit == 0) {
            std::cerr << "height limit reached\n";
            return false;
        }

        // Create a basic block state for this block.
        sym_exec_bb_state bb_state;
        bb_state.prev_bb_state = prev_state;
        bb_state.branch_cond = branch_cond;

#ifdef PRINT_TREE
        {
            static int n_states = 0;
            bb_state.state_id = n_states++;

            if (bb_state.prev_bb_state) {
                ofs_tree << "\"" << bb_state.prev_bb_state->state_id;
                ofs_tree << "\"->\"" << bb_state.state_id << "\"";
                ofs_tree << " [label=\"" << std::boolalpha;
                ofs_tree << bb_state.branch_cond << "\"]\n";
            }

            ofs_tree << "\"" << bb_state.state_id << "\"";
            ofs_tree << "[label=\"";
        }
#endif

        bool satisfiable = true;
        if (prev_state != nullptr && prev_state->branch_expr != nullptr) {
            if (auto c = boost::get<bool>(&branch_cond)) {
                if (*c) {
                    bb_state.path_cond = prev_state->branch_expr;
                }
                else {
                    bb_state.path_cond = std::make_shared<sym_unary_op_expr>(
                            'Z', sym_unary_op_not, prev_state->branch_expr);
                }
            }
            else if (auto c = boost::get<int32_t>(&branch_cond)) {
                bb_state.path_cond = std::make_shared<sym_binary_op_expr>(
                        'I', sym_binary_op_eq, prev_state->branch_expr,
                        std::make_shared<sym_val_expr>('I', *c));
            }
            else {
                // FIXME.
                bb_state.path_cond = std::make_shared<sym_unary_op_expr>(
                        'Z', sym_unary_op_not, prev_state->branch_expr);
            }

            satisfiable = check_satisfiability(bb_state);
        }

#ifdef PRINT_TREE
        {
            if (!satisfiable) {
                ofs_tree << "unsat\"];\n";
            }
        }
#endif

        if (!satisfiable) {
            return false;
        }

        // Create visitors.
        sym_update_state update_state(bb_state, vm, vm.methods()[mv]);
        sym_update_pc update_pc(bb_state);

        bool pc_updated = false;

        // Execute instructions within the basic block.
        size_t iv = head_iv;
        for (;;) {
            const auto& insn = cfg[iv].insn;

            // Record this instruction.
            bb_state.log.push_back(iv);

            // Update the state.
            boost::apply_visitor(update_state, insn);

            // Update the path conditions if it's a branch.
            pc_updated = boost::apply_visitor(update_pc, insn);
            if (pc_updated) {
                // Branch found, and PC is updated.
                break;
            }

            // Get the next instruction.
            const auto& oe = out_edges(iv, cfg);
            if (oe.first == oe.second) {
                // No instruction to follow: assume EXIT.
                sym_make_summary(bb_state);
                break;
            }

            // Move to the next instruction.
            iv = target(*oe.first, cfg);
        }

#ifdef PRINT_TREE
        {
            for (auto v : bb_state.log) {
                ofs_tree << v << ": " << cfg[v].insn << "\\l";
            }
            ofs_tree << "\"]\n";
        }
#endif

        if (pc_updated) {
            const auto& oe = out_edges(iv, cfg);

            for (const auto& e : boost::make_iterator_range(oe)) {
                const auto& target_iv = target(e, cfg);

                // Extract the branch condition.
                namespace te = boost::type_erasure;
                using cfg_edge = insn_control_flow_edge_property;
                const auto& branch_cond
                        = te::any_cast<const cfg_edge&>(cfg[e]).branch_cond;

                run_basic_block(vm, mv, cfg, target_iv, branch_cond, &bb_state,
                                bb_len_limit - 1);
            }
        }

        return true;
    }

    void run_method(virtual_machine& vm, const method_vertex_descriptor& mv)
    {
        const auto& mg = vm.methods();
        const auto& ig = mg[mv].insns;
        const auto& cfg
                = make_edge_filtered_graph<insn_control_flow_edge_property>(ig);

        if (num_vertices(cfg) == 0) {
            return;
        }

        run_basic_block(vm, mv, cfg, 0, insn_control_flow_fallthrough(),
                        nullptr, 20);
    }
}

void jitana::symbolic_execution(virtual_machine& vm,
                                const method_vertex_descriptor& mv)
{
#ifdef PRINT_TREE
    ofs_tree << "digraph {\n";
    // ofs_tree << "rankdir=LR\n";
    ofs_tree << "node[shape=record]\n";
    ofs_tree << "edge[color=blue, fontcolor=blue]\n";
#endif

    yices_init();
    run_method(vm, mv);
    yices_exit();

#ifdef PRINT_TREE
    ofs_tree << "}\n";
#endif
}
