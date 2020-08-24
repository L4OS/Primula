#include "namespace.h"

// This code substitutes a ternary operation by variable and condition

static expression_node_t * FindTernaryOp(expression_node_t * node)
{
    if (node == nullptr)
        return 0;
    if (node->lexem == lt_quest)
        return node;

    expression_node_t * leftright = nullptr;
    if (node->left != nullptr)
    {
        leftright = FindTernaryOp(node->left);
        if (leftright)
            return leftright;
    }
    if (node->right != nullptr)
        leftright = FindTernaryOp(node->right);

    return leftright;
}

static expression_t * CreateAssignment(variable_base_t * var, expression_node_t * expr)
{
    expression_node_t   *   set = new expression_node_t(lt_set);
    set->left = new expression_node_t(lt_variable);
    set->left->variable = var;
    set->right = expr;
    set->type = var->type; // TODO: Check types
    return new expression_t(set);
}

void namespace_t::TranslateTernaryOperation(expression_t  *  code)
{
    linkage_t linkage;

    expression_node_t * ternary = FindTernaryOp(code->root);
    if (ternary != nullptr)
    {
        // TODO: Check types, allocation variables, and embedded ternary operations
        // This is preliminary version of translation

        variable_base_t     *   tern_var = this->FindVariableInspace("ternary");
        if(tern_var == nullptr)
            tern_var = this->CreateVariable(ternary->type, "ternary", FindSegmentType(&linkage));

        expression_node_t   *   condition = ternary->left;
        expression_node_t   *   true_side = ternary->right->left;
        expression_node_t   *   false_side = ternary->right->right;
        
        operator_IF * oper_if = new operator_IF;
        oper_if->expression = new expression_t(condition);
        oper_if->true_statement = CreateAssignment(tern_var, true_side);
        oper_if->false_statement = CreateAssignment(tern_var, false_side);
        this->space_code.push_back(oper_if);

        ternary->lexem = lt_variable;
        ternary->variable = tern_var;
        ternary->left = nullptr;
        ternary->right = nullptr;
    }
}