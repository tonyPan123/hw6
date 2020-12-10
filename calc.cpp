// Authors:
// Jack Zhang (jzhan237)
// Tony Pan (jpan26)
#include "calc.h"
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <pthread.h> 

typedef std::unordered_map<std::string, int> VariableList;

struct Calc {
};

// implementation of Calc
class CalcImpl : public Calc {
public:
  CalcImpl () {
      pthread_mutex_init(&lock, NULL);
    }
    ~CalcImpl () {
      pthread_mutex_destroy(&lock);
    }
    int evalExpr(const char *expr, int *result);
private:
    VariableList varlist;
    pthread_mutex_t lock;

    bool parse_op(std::string token, char *result);
    bool parse_operand(std::string token, int *result);
    bool evaluate(std::vector<std::string> tokens, int *result);
};

std::vector<std::string> tokenize(const std::string &expr) {
    std::vector<std::string> vec;
    std::stringstream s(expr);

    std::string tok;
    while (s >> tok) {
        vec.push_back(tok);
    }

    return vec;
}

bool has_only_digits(const std::string s) {
    return s.find_first_not_of("0123456789") == std::string::npos;
}

bool is_integer(const std::string s) {
    if (s.empty())
        return false;
    if (s[0] == '-')
        return has_only_digits(s.substr(1));
    else
        return has_only_digits(s);
}

bool has_only_alpha(const std::string s){
  return s.find_first_not_of(\
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")==std::string::npos;
}

// Helper function to parse a single operand
bool CalcImpl::parse_operand(std::string token, int *result) {
    // Integer
    if (is_integer(token)) {
        *result = std::stoi(token);
        return true;
    }
    // Variable
    if (has_only_alpha(token)) {
        if (varlist.find(token) == varlist.end()) {
	        return false;
        } // variable is undefined
        *result = varlist[token];
        return true;
    }
    // Mixed number and alpha, not allowed
    return false;
}

// Helper function to parse operator
bool CalcImpl::parse_op(std::string token, char *result) {
    if (token.length() != 1) return false;
    char op = token.c_str()[0];
    if ((op == '+') || (op == '-') || (op == '*') || (op == '/')) {
        *result = op;
        return true;
    }
    else return false;
}

// Evaluate non-assignment type expressions
bool CalcImpl::evaluate(std::vector<std::string> tokens, int *result) {
    int operand1, operand2;
    char op;
    if (tokens.size() == 1) {
        // [operand]
        if (parse_operand(tokens[0], &operand1)) {
            *result = operand1;
            return true;
        } 
        else return false;
    } else if (tokens.size() == 3) {
      
      // [operand op operand]
        if (parse_operand(tokens[0], &operand1) && 
            parse_op(tokens[1], &op) && 
            parse_operand(tokens[2], &operand2)) {
            switch (op) {
            case '+':
                *result = operand1+operand2;
                return true;
            case '-':
                *result = operand1-operand2;
                return true;
            case '*':
                *result = operand1*operand2;
                return true;
            case '/':
                if (operand2 == 0) return false;
                *result = operand1/operand2;
                return true;    
            default:
                return false;
            }
        }
        else return false;
    }
    else return false;
}

int CalcImpl::evalExpr(const char *expr, int *result) {
    std::string expr_str(expr);
    std::vector<std::string> tokens = tokenize(expr_str);
    std::string equality_sign("=");

    if (std::find(tokens.begin(), tokens.end(), equality_sign)==tokens.end()) {
        // not an assignment operation
        return evaluate(tokens, result);
    } else {
        // is an assignment operation
        if (tokens.size() < 3)
            return false;
        if (!has_only_alpha(tokens[0]) || tokens[1] != equality_sign)
            return false;
        std::vector<std::string> new_tokens;
        for (unsigned i = 2; i < tokens.size(); i++) {
            // get subvector starting at index 2
            new_tokens.push_back(tokens[i]);
        }

        int temp_result;
        // treat the subvector as a non-assigment operation fisrt
        // then do assignment if there is a valid result
        pthread_mutex_lock(&lock);
        if (evaluate(new_tokens, &temp_result)){
            varlist[tokens[0]] = temp_result; // assign to varlist
            pthread_mutex_unlock(&lock);
            *result = temp_result;
            return true;
        }
        else {
            pthread_mutex_unlock(&lock);
            return false;
        }
    }
}

extern "C" struct Calc *calc_create(void) {
    return new CalcImpl();
}

extern "C" void calc_destroy(struct Calc *calc) {
    CalcImpl *obj = static_cast<CalcImpl *>(calc);
    delete obj;
}

extern "C" int calc_eval(struct Calc *calc, const char *expr, int *result) {
    CalcImpl *obj = static_cast<CalcImpl *>(calc);
    return obj->evalExpr(expr, result);
}
