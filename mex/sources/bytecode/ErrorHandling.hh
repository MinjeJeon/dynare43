/*
 * Copyright © 2007-2021 Dynare Team
 *
 * This file is part of Dynare.
 *
 * Dynare is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Dynare is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Dynare.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ERROR_HANDLING
#define ERROR_HANDLING

#include <vector>
#include <utility>
#include <string>
#include <map>
#include <tuple>
#include <cstddef>
#include <sstream>
#include <iostream>
#include <stack>
#define _USE_MATH_DEFINES
#include <cmath>

#include "dynmex.h"

#define BYTE_CODE
#include "CodeInterpreter.hh"

#ifdef OCTAVE_MEX_FILE
# define CHAR_LENGTH 1
#else
# define CHAR_LENGTH 2
#endif

using namespace std;

constexpr int NO_ERROR_ON_EXIT = 0, ERROR_ON_EXIT = 1;

using code_liste_type = vector<pair<Tags, void *>>;
using it_code_type = code_liste_type::const_iterator;

class GeneralExceptionHandling
{
  string ErrorMsg;
public:
  GeneralExceptionHandling(string ErrorMsg_arg) : ErrorMsg{move(ErrorMsg_arg)}
  {
  };
  inline string
  GetErrorMsg()
  {
    return ErrorMsg;
  }
  inline void
  completeErrorMsg(const string &ErrorMsg_arg)
  {
    ErrorMsg += ErrorMsg_arg;
  }
};

class FloatingPointExceptionHandling : public GeneralExceptionHandling
{
public:
  FloatingPointExceptionHandling(const string &value) : GeneralExceptionHandling("Floating point error in bytecode: " + value)
  {
  }
};

class LogExceptionHandling : public FloatingPointExceptionHandling
{
  double value;
public:
  LogExceptionHandling(double value_arg) : FloatingPointExceptionHandling("log(X)"),
                                           value(value_arg)
  {
    completeErrorMsg(" with X=" + to_string(value) + "\n");
  }
};

class Log10ExceptionHandling : public FloatingPointExceptionHandling
{
  double value;
public:
  Log10ExceptionHandling(double value_arg) : FloatingPointExceptionHandling("log10(X)"),
                                             value(value_arg)
  {
    completeErrorMsg(" with X=" + to_string(value) + "\n");
  }
};

class DivideExceptionHandling : public FloatingPointExceptionHandling
{
  double value1, value2;
public:
  DivideExceptionHandling(double value1_arg, double value2_arg) : FloatingPointExceptionHandling("a/X"),
                                                                  value1(value1_arg),
                                                                  value2(value2_arg)
  {
    completeErrorMsg(" with X=" + to_string(value2) + "\n");
  }
};

class PowExceptionHandling : public FloatingPointExceptionHandling
{
  double value1, value2;
public:
  PowExceptionHandling(double value1_arg, double value2_arg) : FloatingPointExceptionHandling("X^a"),
                                                               value1(value1_arg),
                                                               value2(value2_arg)
  {
    if (fabs(value1) > 1e-10)
      completeErrorMsg(" with X=" + to_string(value1) + "\n");
    else
      completeErrorMsg(" with X=" + to_string(value1) + " and a=" + to_string(value2) + "\n");
  };
};

class UserExceptionHandling : public GeneralExceptionHandling
{
  double value;
public:
  UserExceptionHandling() : GeneralExceptionHandling("Fatal error in bytecode:")
  {
    completeErrorMsg(" User break\n");
  };
};

class FatalExceptionHandling : public GeneralExceptionHandling
{
public:
  FatalExceptionHandling(const string &ErrorMsg_arg)
    : GeneralExceptionHandling("Fatal error in bytecode:")
  {
    completeErrorMsg(ErrorMsg_arg);
  };
  FatalExceptionHandling() : GeneralExceptionHandling("")
  {
  };
};

struct s_plan
{
  string var, exo;
  int var_num, exo_num;
  vector<pair<int, double>> per_value;
  vector<double> value;
};

struct table_conditional_local_type
{
  bool is_cond;
  int var_exo, var_endo;
  double constrained_value;
};
using vector_table_conditional_local_type = vector<table_conditional_local_type>;
using table_conditional_global_type = map<int, vector_table_conditional_local_type>;
#ifdef MATLAB_MEX_FILE
extern "C" bool utIsInterruptPending();
#endif

class ErrorMsg
{
private:
  bool is_load_variable_list;

public:
  double *y, *ya;
  int y_size;
  double *T;
  int nb_row_xd, nb_row_x, col_x, col_y;
  int y_kmin, y_kmax, periods;
  double *x, *params;
  double *u;
  double *steady_y, *steady_x;
  double *g2, *g1, *r, *res;
  vector<s_plan> splan, spfplan;
  vector<mxArray *> jacobian_block, jacobian_other_endo_block, jacobian_exo_block, jacobian_det_exo_block;
  map<unsigned int, double> TEF;
  map<pair<unsigned int, unsigned int>, double> TEFD;
  map<tuple<unsigned int, unsigned int, unsigned int>, double> TEFDD;

  ExpressionType EQN_type;
  it_code_type it_code_expr;
  size_t nb_endo, nb_exo, nb_param;
  char *P_endo_names, *P_exo_names, *P_param_names;
  size_t endo_name_length, exo_name_length, param_name_length;
  unsigned int EQN_equation, EQN_block, EQN_block_number;
  unsigned int EQN_dvar1, EQN_dvar2, EQN_dvar3;
  vector<tuple<string, SymbolType, unsigned int>> Variable_list;

  inline
  ErrorMsg()
  {
    mxArray *M_ = mexGetVariable("global", "M_");
    if (mxGetFieldNumber(M_, "endo_names") == -1)
      {
        nb_endo = 0;
        endo_name_length = 0;
        P_endo_names = nullptr;
      }
    else
      {
        nb_endo = mxGetM(mxGetFieldByNumber(M_, 0, mxGetFieldNumber(M_, "endo_names")));
        endo_name_length = mxGetN(mxGetFieldByNumber(M_, 0, mxGetFieldNumber(M_, "endo_names")));
        P_endo_names = reinterpret_cast<char *>(mxGetPr(mxGetFieldByNumber(M_, 0, mxGetFieldNumber(M_, "endo_names"))));
      }
    if (mxGetFieldNumber(M_, "exo_names") == -1)
      {
        nb_exo = 0;
        exo_name_length = 0;
        P_exo_names = nullptr;
      }
    else
      {
        nb_exo = mxGetM(mxGetFieldByNumber(M_, 0, mxGetFieldNumber(M_, "exo_names")));
        exo_name_length = mxGetN(mxGetFieldByNumber(M_, 0, mxGetFieldNumber(M_, "exo_names")));
        P_exo_names = reinterpret_cast<char *>(mxGetPr(mxGetFieldByNumber(M_, 0, mxGetFieldNumber(M_, "exo_names"))));
      }
    if (mxGetFieldNumber(M_, "param_names") == -1)
      {
        nb_param = 0;
        param_name_length = 0;
        P_param_names = nullptr;
      }
    else
      {
        nb_param = mxGetM(mxGetFieldByNumber(M_, 0, mxGetFieldNumber(M_, "param_names")));
        param_name_length = mxGetN(mxGetFieldByNumber(M_, 0, mxGetFieldNumber(M_, "param_names")));
        P_param_names = reinterpret_cast<char *>(mxGetPr(mxGetFieldByNumber(M_, 0, mxGetFieldNumber(M_, "param_names"))));
      }
    is_load_variable_list = false;
  }

  inline string
  add_underscore_to_fpe(const string &str)
  {
    string temp;
    int pos1 = -1, pos2 = -1;
    string tmp_n(str.length(), ' ');
    string dollar{"$"}, pound{"£"}, tilde{"~"};
    for (const char & i : str)
      {
        if (dollar.compare(&i) != 0 && pound.compare(&i) != 0)
          temp += i;
        else
          {
            if (dollar.compare(&i) == 0)
              pos1 = static_cast<int>(temp.length());
            else
              pos2 = static_cast<int>(temp.length());
            if (pos1 >= 0 && pos2 >= 0)
              {
                tmp_n.erase(pos1, pos2-pos1+1);
                tmp_n.insert(pos1, pos2-pos1, tilde[0]);
                pos1 = pos2 = -1;
              }
          }
      }
    temp += "\n" + tmp_n;
    return temp;
  }

  inline void
  load_variable_list()
  {
    ostringstream res;
    for (unsigned int variable_num = 0; variable_num < static_cast<unsigned int>(nb_endo); variable_num++)
      {
        for (unsigned int i = 0; i < endo_name_length; i++)
          if (P_endo_names[CHAR_LENGTH*(variable_num+i*nb_endo)] != ' ')
            res << P_endo_names[CHAR_LENGTH*(variable_num+i*nb_endo)];
        Variable_list.emplace_back(res.str(), SymbolType::endogenous, variable_num);
      }
    for (unsigned int variable_num = 0; variable_num < static_cast<unsigned int>(nb_exo); variable_num++)
      {
        for (unsigned int i = 0; i < exo_name_length; i++)
          if (P_exo_names[CHAR_LENGTH*(variable_num+i*nb_exo)] != ' ')
            res << P_exo_names[CHAR_LENGTH*(variable_num+i*nb_exo)];
        Variable_list.emplace_back(res.str(), SymbolType::exogenous, variable_num);
      }
  }

  inline int
  get_ID(const string &variable_name, SymbolType *variable_type)
  {
    if (!is_load_variable_list)
      {
        load_variable_list();
        is_load_variable_list = true;
      }
    size_t n = Variable_list.size();
    int i = 0;
    bool notfound = true;
    while (notfound && i < static_cast<int>(n))
      {
        if (variable_name == get<0>(Variable_list[i]))
          {
            notfound = false;
            *variable_type = get<1>(Variable_list[i]);
            return get<2>(Variable_list[i]);
          }
        i++;
      }
    return -1;
  }

  inline string
  get_variable(SymbolType variable_type, unsigned int variable_num) const
  {
    ostringstream res;
    switch (variable_type)
      {
      case SymbolType::endogenous:
        if (variable_num <= nb_endo)
          {
            for (unsigned int i = 0; i < endo_name_length; i++)
              if (P_endo_names[CHAR_LENGTH*(variable_num+i*nb_endo)] != ' ')
                res << P_endo_names[CHAR_LENGTH*(variable_num+i*nb_endo)];
          }
        else
          mexPrintf("=> Unknown endogenous variable # %d", variable_num);
        break;
      case SymbolType::exogenous:
      case SymbolType::exogenousDet:
        if (variable_num <= nb_exo)
          {
            for (unsigned int i = 0; i < exo_name_length; i++)
              if (P_exo_names[CHAR_LENGTH*(variable_num+i*nb_exo)] != ' ')
                res << P_exo_names[CHAR_LENGTH*(variable_num+i*nb_exo)];
          }
        else
          mexPrintf("=> Unknown exogenous variable # %d", variable_num);
        break;
      case SymbolType::parameter:
        if (variable_num <= nb_param)
          {
            for (unsigned int i = 0; i < param_name_length; i++)
              if (P_param_names[CHAR_LENGTH*(variable_num+i*nb_param)] != ' ')
                res << P_param_names[CHAR_LENGTH*(variable_num+i*nb_param)];
          }
        else
          mexPrintf("=> Unknown parameter # %d", variable_num);
        break;
      default:
        break;
      }
    return res.str();
  }

  inline string
  error_location(bool evaluate, bool steady_state, int size, int block_num, int it_, int Per_u_)
  {
    ostringstream Error_loc;
    if (!steady_state)
      switch (EQN_type)
        {
        case ExpressionType::TemporaryTerm:
          if (EQN_block_number > 1)
            Error_loc << "temporary term " << EQN_equation+1 << " in block " << EQN_block+1 << " at time " << it_;
          else
            Error_loc << "temporary term " << EQN_equation+1 << " at time " << it_;
          break;
        case ExpressionType::ModelEquation:
          if (EQN_block_number > 1)
            Error_loc << "equation " << EQN_equation+1 << " in block " << EQN_block+1 << " at time " << it_;
          else
            Error_loc << "equation " << EQN_equation+1 << " at time " << it_;
          break;
        case ExpressionType::FirstEndoDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to endogenous variable " << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to endogenous variable " << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          break;
        case ExpressionType::FirstOtherEndoDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to other endogenous variable "  << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to other endogenous variable " << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          break;
        case ExpressionType::FirstExoDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to exogenous variable "  << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to exogenous variable " << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          break;
        case ExpressionType::FirstExodetDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to deterministic exogenous variable "  << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to deterministic exogenous variable " << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          break;
        case ExpressionType::FirstParamDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to parameter "  << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to parameter " << get_variable(SymbolType::endogenous, EQN_dvar1) << " at time " << it_;
          break;
        default:
          return "???";
        }
    else
      switch (EQN_type)
        {
        case ExpressionType::TemporaryTerm:
          if (EQN_block_number > 1)
            Error_loc << "temporary term " << EQN_equation+1 << " in block " << EQN_block+1;
          else
            Error_loc << "temporary term " << EQN_equation+1;
          break;
        case ExpressionType::ModelEquation:
          if (EQN_block_number > 1)
            Error_loc << "equation " << EQN_equation+1 << " in block " << EQN_block+1;
          else
            Error_loc << "equation " << EQN_equation+1;
          break;
        case ExpressionType::FirstEndoDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to endogenous variable "  << get_variable(SymbolType::endogenous, EQN_dvar1);
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to endogenous variable " << get_variable(SymbolType::endogenous, EQN_dvar1);
          break;
        case ExpressionType::FirstOtherEndoDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to other endogenous variable "  << get_variable(SymbolType::endogenous, EQN_dvar1);
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to other endogenous variable " << get_variable(SymbolType::endogenous, EQN_dvar1);
          break;
        case ExpressionType::FirstExoDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to exogenous variable "  << get_variable(SymbolType::endogenous, EQN_dvar1);
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to exogenous variable " << get_variable(SymbolType::endogenous, EQN_dvar1);
          break;
        case ExpressionType::FirstExodetDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to deterministic exogenous variable "  << get_variable(SymbolType::endogenous, EQN_dvar1);
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to deterministic exogenous variable " << get_variable(SymbolType::endogenous, EQN_dvar1);
          break;
        case ExpressionType::FirstParamDerivative:
          if (EQN_block_number > 1)
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " in block " << EQN_block+1 << " with respect to parameter "  << get_variable(SymbolType::endogenous, EQN_dvar1);
          else
            Error_loc << "first order derivative of equation " << EQN_equation+1 << " with respect to parameter " << get_variable(SymbolType::endogenous, EQN_dvar1);
          break;
        default:
          return ("???");
        }
    it_code_type it_code_ret;
    Error_loc << endl << add_underscore_to_fpe("      " + print_expression(it_code_expr, evaluate, size, block_num, steady_state, Per_u_, it_, it_code_ret, true));
    return Error_loc.str();
  }

  inline string
  print_expression(it_code_type it_code, bool evaluate, int size, int block_num, bool steady_state, int Per_u_, int it_, it_code_type &it_code_ret, bool compute) const
  {
    int var, lag = 0, op, eq;
    stack<string> Stack;
    stack<double> Stackf;
    ostringstream tmp_out, tmp_out2;
    string v1, v2, v3;
    double v1f, v2f, v3f = 0.0;
    bool go_on = true;
    double ll;
    ExpressionType equation_type = ExpressionType::TemporaryTerm;
    size_t found;
    double *jacob = nullptr, *jacob_other_endo = nullptr, *jacob_exo = nullptr, *jacob_exo_det = nullptr;
    ExternalFunctionType function_type = ExternalFunctionType::withoutDerivative;

    if (evaluate)
      {
        jacob = mxGetPr(jacobian_block[block_num]);
        if (!steady_state)
          {
            jacob_other_endo = mxGetPr(jacobian_other_endo_block[block_num]);
            jacob_exo = mxGetPr(jacobian_exo_block[block_num]);
            jacob_exo_det = mxGetPr(jacobian_det_exo_block[block_num]);
          }
      }

    while (go_on)
      {
#ifdef MATLAB_MEX_FILE
        if (utIsInterruptPending())
          throw UserExceptionHandling();
#endif
        switch (it_code->first)
          {
          case Tags::FNUMEXPR:
            switch (static_cast<FNUMEXPR_ *>(it_code->second)->get_expression_type())
              {
              case ExpressionType::TemporaryTerm:
                equation_type = ExpressionType::TemporaryTerm;
                break;
              case ExpressionType::ModelEquation:
                equation_type = ExpressionType::ModelEquation;
                break;
              case ExpressionType::FirstEndoDerivative:
                equation_type = ExpressionType::FirstEndoDerivative;
                break;
              case ExpressionType::FirstOtherEndoDerivative:
                equation_type = ExpressionType::FirstOtherEndoDerivative;
                break;
              case ExpressionType::FirstExoDerivative:
                equation_type = ExpressionType::FirstExoDerivative;
                break;
              case ExpressionType::FirstExodetDerivative:
                equation_type = ExpressionType::FirstExodetDerivative;
                break;
              case ExpressionType::FirstParamDerivative:
                equation_type = ExpressionType::FirstParamDerivative;
                break;
              case ExpressionType::SecondEndoDerivative:
                equation_type = ExpressionType::SecondEndoDerivative;
                break;
              case ExpressionType::SecondExoDerivative:
                equation_type = ExpressionType::SecondExoDerivative;
                break;
              case ExpressionType::SecondExodetDerivative:
                equation_type = ExpressionType::SecondExodetDerivative;
                break;
              case ExpressionType::SecondParamDerivative:
                equation_type = ExpressionType::SecondExodetDerivative;
                break;
              case ExpressionType::ThirdEndoDerivative:
                equation_type = ExpressionType::ThirdEndoDerivative;
                break;
              case ExpressionType::ThirdExoDerivative:
                equation_type = ExpressionType::ThirdExoDerivative;
                break;
              case ExpressionType::ThirdExodetDerivative:
                equation_type = ExpressionType::ThirdExodetDerivative;
                break;
              case ExpressionType::ThirdParamDerivative:
                equation_type = ExpressionType::ThirdExodetDerivative;
                break;
              default:
                ostringstream tmp;
                tmp << " in print_expression, expression type " << static_cast<int>(static_cast<FNUMEXPR_ *>(it_code->second)->get_expression_type()) << " not implemented yet\n";
                throw FatalExceptionHandling(tmp.str());
              }
            break;
          case Tags::FLDV:
            //load a variable in the processor
            switch (static_cast<SymbolType>(static_cast<FLDV_ *>(it_code->second)->get_type()))
              {
              case SymbolType::parameter:
                var = static_cast<FLDV_ *>(it_code->second)->get_pos();
#ifdef DEBUG
                mexPrintf("FLDV_ Param var=%d\n", var);
                mexPrintf("get_variable(SymbolType::parameter, var)=%s\n", get_variable(SymbolType::parameter, var).c_str());
                mexEvalString("drawnow;");
#endif
                Stack.push(get_variable(SymbolType::parameter, var));
                if (compute)
                  Stackf.push(params[var]);
                break;
              case SymbolType::endogenous:
                var = static_cast<FLDV_ *>(it_code->second)->get_pos();
                lag = static_cast<FLDV_ *>(it_code->second)->get_lead_lag();
#ifdef DEBUG
                mexPrintf("FLDV_ endo var=%d, lag=%d\n", var, lag);
                mexPrintf("get_variable(SymbolType::endogenous, var)=%s, compute=%d\n", get_variable(SymbolType::endogenous, var).c_str(), compute);
                mexPrintf("it_=%d, lag=%d, y_size=%d, var=%d, y=%x\n", it_, lag, y_size, var, y);
                mexEvalString("drawnow;");
#endif
                tmp_out.str("");
                if (lag > 0)
                  tmp_out << get_variable(SymbolType::endogenous, var) << "(+" << lag << ")";
                else if (lag < 0)
                  tmp_out << get_variable(SymbolType::endogenous, var) << "(" << lag << ")";
                else
                  tmp_out << get_variable(SymbolType::endogenous, var);
                Stack.push(tmp_out.str());
                if (compute)
                  {
                    if (evaluate)
                      Stackf.push(ya[(it_+lag)*y_size+var]);
                    else
                      Stackf.push(y[(it_+lag)*y_size+var]);
                  }
                break;
              case SymbolType::exogenous:
                var = static_cast<FLDV_ *>(it_code->second)->get_pos();
                lag = static_cast<FLDV_ *>(it_code->second)->get_lead_lag();
#ifdef DEBUG
                mexPrintf("FLDV_ exo var=%d, lag=%d", var, lag);
#endif
                tmp_out.str("");
                if (lag != 0)
                  tmp_out << get_variable(SymbolType::exogenous, var) << "(" << lag << ")";
                else
                  tmp_out << get_variable(SymbolType::exogenous, var);
                Stack.push(tmp_out.str());
                if (compute)
                  Stackf.push(x[it_+lag+var*nb_row_x]);
                break;
              case SymbolType::exogenousDet:
                var = static_cast<FLDV_ *>(it_code->second)->get_pos();
                lag = static_cast<FLDV_ *>(it_code->second)->get_lead_lag();
#ifdef DEBUG
                mexPrintf("FLDV_ exo_det var=%d, lag=%d", var, lag);
#endif
                tmp_out.str("");
                if (lag != 0)
                  tmp_out << get_variable(SymbolType::exogenousDet, var) << "(" << lag << ")";
                else
                  tmp_out << get_variable(SymbolType::exogenousDet, var);
                Stack.push(tmp_out.str());
                if (compute)
                  Stackf.push(x[it_+lag+var*nb_row_xd]);
                break;
              case SymbolType::modelLocalVariable:
                break;
              default:
                mexPrintf("FLDV: Unknown variable type\n");
              }
            break;
          case Tags::FLDSV:
          case Tags::FLDVS:
            //load a variable in the processor
            switch (static_cast<SymbolType>(static_cast<FLDSV_ *>(it_code->second)->get_type()))
              {
              case SymbolType::parameter:
                var = static_cast<FLDSV_ *>(it_code->second)->get_pos();
#ifdef DEBUG
                mexPrintf("FLDSV_ param var=%d", var);
#endif
                Stack.push(get_variable(SymbolType::parameter, var));
                if (compute)
                  Stackf.push(params[var]);
                break;
              case SymbolType::endogenous:
                var = static_cast<FLDSV_ *>(it_code->second)->get_pos();
#ifdef DEBUG
                mexPrintf("FLDSV_ endo var=%d", var);
#endif
                Stack.push(get_variable(SymbolType::endogenous, var));
                if (compute)
                  {
                    if (it_code->first == Tags::FLDSV)
                      {
                        if (evaluate)
                          Stackf.push(ya[var]);
                        else
                          Stackf.push(y[var]);
                      }
                    else
                      Stackf.push(steady_y[var]);
                  }
                break;
              case SymbolType::exogenous:
                var = static_cast<FLDSV_ *>(it_code->second)->get_pos();
#ifdef DEBUG
                mexPrintf("FLDSV_ exo var=%d", var);
#endif
                Stack.push(get_variable(SymbolType::exogenous, var));
#ifdef DEBUG
                mexPrintf("oka var=%d, Stack.size()=%d x=%x\n", var, Stack.size(), x);
#endif
                if (compute)
                  Stackf.push(x[var]);
                break;
              case SymbolType::exogenousDet:
                var = static_cast<FLDSV_ *>(it_code->second)->get_pos();
#ifdef DEBUG
                mexPrintf("FLDSV_ exo_det var=%d", var);
#endif
                Stack.push(get_variable(SymbolType::exogenousDet, var));
                if (compute)
                  Stackf.push(x[var]);
                break;
              case SymbolType::modelLocalVariable:
                break;
              default:
                mexPrintf("FLDSV: Unknown variable type\n");
              }
            break;
          case Tags::FLDT:
            //load a temporary variable in the processor
            var = static_cast<FLDT_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FLDT_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "T" << var+1;
            Stack.push(tmp_out.str());
            if (compute)
              Stackf.push(T[var*(periods+y_kmin+y_kmax)+it_]);
            break;
          case Tags::FLDST:
            //load a temporary variable in the processor
            var = static_cast<FLDST_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FLDST_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "T" << var+1;
            Stack.push(tmp_out.str());
            if (compute)
              Stackf.push(T[var]);
            break;
          case Tags::FLDU:
            //load u variable in the processor
            var = static_cast<FLDU_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FLDU_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "u(" << var+1 << " + it_)";
            Stack.push(tmp_out.str());
            var += Per_u_;
            if (compute)
              Stackf.push(u[var]);
            break;
          case Tags::FLDSU:
            //load u variable in the processor
            var = static_cast<FLDSU_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FLDSU_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "u(" << var+1 << ")";
            Stack.push(tmp_out.str());
            if (compute)
              Stackf.push(u[var]);
            break;
          case Tags::FLDR:
            var = static_cast<FLDR_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FLDSR_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "residual(" << var+1 << ")";
            Stack.push(tmp_out.str());
            if (compute)
              Stackf.push(r[var]);
            break;
          case Tags::FLDZ:
            //load 0 in the processor
#ifdef DEBUG
            mexPrintf("FLDZ_");
#endif
            tmp_out.str("");
            tmp_out << 0;
            Stack.push(tmp_out.str());
            if (compute)
              Stackf.push(0.0);
            break;
          case Tags::FLDC:
            //load a numerical constant in the processor
            ll = static_cast<FLDC_ *>(it_code->second)->get_value();
            tmp_out.str("");
#ifdef DEBUG
            mexPrintf("FLDC_ ll=%f", ll);
#endif
            tmp_out << ll;
            Stack.push(tmp_out.str());
            if (compute)
              Stackf.push(ll);
            break;
          case Tags::FSTPV:
            //load a variable in the processor
            go_on = false;
            switch (static_cast<SymbolType>(static_cast<FSTPV_ *>(it_code->second)->get_type()))
              {
              case SymbolType::parameter:
                var = static_cast<FSTPV_ *>(it_code->second)->get_pos();
#ifdef DEBUG
                mexPrintf("FSTPV_ param var=%d", var);
#endif
                tmp_out2.str("");
                tmp_out2 << Stack.top();
                tmp_out.str("");
                tmp_out << get_variable(SymbolType::parameter, var) << " = " << tmp_out2.str();
                Stack.pop();
                if (compute)
                  {
                    params[var] = Stackf.top();
                    Stackf.pop();
                  }
                break;
              case SymbolType::endogenous:
                var = static_cast<FSTPV_ *>(it_code->second)->get_pos();
                lag = static_cast<FSTPV_ *>(it_code->second)->get_lead_lag();
#ifdef DEBUG
                mexPrintf("FSTPV_ endo var=%d, lag=%d", var, lag);
#endif
                tmp_out2.str("");
                tmp_out2 << Stack.top();
                tmp_out.str("");
                tmp_out << get_variable(SymbolType::endogenous, var);
                if (lag > 0)
                  tmp_out << "(+" << lag << ")";
                else if (lag < 0)
                  tmp_out << "(" << lag << ")";
                tmp_out << " = " << tmp_out2.str();
                Stack.pop();
                if (compute)
                  {
                    y[(it_+lag)*y_size+var] = Stackf.top();
                    Stackf.pop();
                  }
                break;
              case SymbolType::exogenous:
                var = static_cast<FSTPV_ *>(it_code->second)->get_pos();
                lag = static_cast<FSTPV_ *>(it_code->second)->get_lead_lag();
#ifdef DEBUG
                mexPrintf("FSTPV_ exo var=%d, lag=%d", var, lag);
#endif
                tmp_out2.str("");
                tmp_out2 << Stack.top();
                tmp_out.str("");
                tmp_out << get_variable(SymbolType::exogenous, var);
                if (lag != 0)
                  tmp_out << "(" << lag << ")";
                tmp_out << " = " << tmp_out2.str();
                Stack.pop();
                if (compute)
                  {
                    x[it_+lag+var*nb_row_x] = Stackf.top();
                    Stackf.pop();
                  }
                break;
              case SymbolType::exogenousDet:
                var = static_cast<FSTPV_ *>(it_code->second)->get_pos();
                lag = static_cast<FSTPV_ *>(it_code->second)->get_lead_lag();
#ifdef DEBUG
                mexPrintf("FSTPV_ exodet var=%d, lag=%d", var, lag);
#endif
                tmp_out2.str("");
                tmp_out2 << Stack.top();
                tmp_out.str("");
                tmp_out << get_variable(SymbolType::exogenousDet, var);
                if (lag != 0)
                  tmp_out << "(" << lag << ")";
                tmp_out << " = " << tmp_out2.str();
                Stack.pop();
                if (compute)
                  {
                    x[it_+lag+var*nb_row_xd] = Stackf.top();
                    Stackf.pop();
                  }
                break;
              default:
                mexPrintf("FSTPV: Unknown variable type\n");
              }
            break;
          case Tags::FSTPSV:
            go_on = false;
            //load a variable in the processor
            switch (static_cast<SymbolType>(static_cast<FSTPSV_ *>(it_code->second)->get_type()))
              {
              case SymbolType::parameter:
                var = static_cast<FSTPSV_ *>(it_code->second)->get_pos();
#ifdef DEBUG
                mexPrintf("FSTPSV_ param var=%d", var);
#endif
                tmp_out2.str("");
                tmp_out2 << Stack.top();
                tmp_out.str("");
                tmp_out << get_variable(SymbolType::parameter, var);
                tmp_out << " = " << tmp_out2.str();
                Stack.pop();
                if (compute)
                  {
                    params[var] = Stackf.top();
                    Stackf.pop();
                  }
                break;
              case SymbolType::endogenous:
                var = static_cast<FSTPSV_ *>(it_code->second)->get_pos();
#ifdef DEBUG
                mexPrintf("FSTPSV_ endo var=%d", var);
#endif
                tmp_out2.str("");
                tmp_out2 << Stack.top();
                tmp_out.str("");
                tmp_out << get_variable(SymbolType::endogenous, var);
                tmp_out << " = " << tmp_out2.str();
                Stack.pop();
                if (compute)
                  {
                    y[var] = Stackf.top();
                    Stackf.pop();
                  }
                break;
              case SymbolType::exogenous:
              case SymbolType::exogenousDet:
                var = static_cast<FSTPSV_ *>(it_code->second)->get_pos();
#ifdef DEBUG
                mexPrintf("FSTPSV_ exo var=%d", var);
#endif
                tmp_out2.str("");
                tmp_out2 << Stack.top();
                tmp_out.str("");
                tmp_out << get_variable(SymbolType::exogenous, var);
                tmp_out << " = " << tmp_out2.str();
                Stack.pop();
                if (compute)
                  {
                    x[var] = Stackf.top();
                    Stackf.pop();
                  }
                break;
              default:
                mexPrintf("FSTPSV: Unknown variable type\n");
              }
            break;
          case Tags::FSTPT:
            go_on = false;
            //store in a temporary variable from the processor
            var = static_cast<FSTPT_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FSTPT_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "T" << var+1 << " = " << Stack.top();
            Stack.pop();
            if (compute)
              {
                T[var*(periods+y_kmin+y_kmax)+it_] = Stackf.top();
                Stackf.pop();
              }
            break;
          case Tags::FSTPST:
            go_on = false;
            //store in a temporary variable from the processor
            var = static_cast<FSTPST_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FSTPST_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "T" << var+1 << " = " << Stack.top();
            Stack.pop();
            if (compute)
              {
                T[var] = Stackf.top();
                Stackf.pop();
              }
            break;
          case Tags::FSTPU:
            go_on = false;
            //store in u variable from the processor
            var = static_cast<FSTPU_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FSTPU_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "u(" << var+1 << " + it_) = " << Stack.top();
            var += Per_u_;
            Stack.pop();
            if (compute)
              {
                u[var] = Stackf.top();
                Stackf.pop();
              }
            break;
          case Tags::FSTPSU:
            go_on = false;
            //store in u variable from the processor
            var = static_cast<FSTPSU_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FSTPSU_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "u(" << var+1 << ") = " << Stack.top();
            Stack.pop();
            if (compute)
              {
                u[var] = Stackf.top();
                Stackf.pop();
              }
            break;
          case Tags::FSTPR:
            go_on = false;
            //store in residual variable from the processor
            var = static_cast<FSTPR_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FSTPR_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "residual(" << var+1 << ") = " << Stack.top();
            Stack.pop();
            if (compute)
              {
                r[var] = Stackf.top();
                Stackf.pop();
              }
            break;
          case Tags::FSTPG:
            go_on = false;
            //store in derivative (g) variable from the processor
            var = static_cast<FSTPG_ *>(it_code->second)->get_pos();
#ifdef DEBUG
            mexPrintf("FSTG_ var=%d", var);
#endif
            tmp_out.str("");
            tmp_out << "g1[" << var+1 << "] = " << Stack.top();
            Stack.pop();
            if (compute)
              {
                g1[var] = Stackf.top();
                Stackf.pop();
              }
            break;
          case Tags::FSTPG2:
            go_on = false;
            //store in derivative (g) variable from the processor
            eq = static_cast<FSTPG2_ *>(it_code->second)->get_row();
            var = static_cast<FSTPG2_ *>(it_code->second)->get_col();
#ifdef DEBUG
            mexPrintf("FSTG2_ eq=%d var=%d", eq, var);
#endif
            tmp_out.str("");
            tmp_out << "/*jacob(" << eq << ", " << var << ")*/ jacob(" << eq+size*var+1 << ") = " << Stack.top();
            Stack.pop();
            if (compute)
              {
                jacob[eq + size*var] = Stackf.top();
                Stackf.pop();
              }
            break;
          case Tags::FSTPG3:
            //store in derivative (g) variable from the processor
#ifdef DEBUG
            mexPrintf("FSTPG3\n");
            mexEvalString("drawnow;");
#endif
            double r;
            unsigned int pos_col;
            go_on = false;
            if (compute)
              {
                r = Stackf.top();
                Stackf.pop();
              }
            eq = static_cast<FSTPG3_ *>(it_code->second)->get_row();
            var = static_cast<FSTPG3_ *>(it_code->second)->get_col();
            lag = static_cast<FSTPG3_ *>(it_code->second)->get_lag();
            pos_col = static_cast<FSTPG3_ *>(it_code->second)->get_col_pos();
            switch (equation_type)
              {
              case ExpressionType::FirstEndoDerivative:
#ifdef DEBUG
                mexPrintf("Endo eq=%d, pos_col=%d, size=%d\n", eq, pos_col, size);
#endif
                if (compute)
                  jacob[eq + size*pos_col] = r;
                tmp_out.str("");
                tmp_out << "/*jacob(" << eq << ", " << pos_col << " var= " << var << ")*/ jacob(" << eq+size*pos_col+1 << ") = " << Stack.top();
                Stack.pop();
                break;
              case ExpressionType::FirstOtherEndoDerivative:
                if (compute)
                  jacob_other_endo[eq + size*pos_col] = r;
                tmp_out.str("");
                tmp_out << "jacob_other_endo(" << eq+size*pos_col+1 << ") = " << Stack.top();
                Stack.pop();
                break;
              case ExpressionType::FirstExoDerivative:
#ifdef DEBUG
                mexPrintf("Exo eq=%d, pos_col=%d, size=%d\n", eq, pos_col, size);
#endif
                if (compute)
                  jacob_exo[eq + size*pos_col] = r;
                tmp_out.str("");
                tmp_out << "/*jacob_exo(" << eq << ", " << pos_col << " var=" << var << ")*/ jacob_exo(" << eq+size*pos_col+1 << ") = " << Stack.top();
                Stack.pop();
                break;
              case ExpressionType::FirstExodetDerivative:
                if (compute)
                  jacob_exo_det[eq + size*pos_col] = r;
                tmp_out.str("");
                tmp_out << "/*jacob_exo_det(" << eq << ", " << pos_col << " var=" << var << ")*/ jacob_exo_det(" << eq+size*pos_col+1 << ") = " << Stack.top();
                Stack.pop();
                break;
              default:
                ostringstream tmp;
                tmp << " in compute_block_time, variable " << static_cast<int>(EQN_type) << " not used yet\n";
                //throw FatalExceptionHandling(tmp.str());
                mexPrintf("%s", tmp.str().c_str());
              }
#ifdef DEBUG
            tmp_out << "=>";
            mexPrintf(" g1[%d](%f)=%s\n", var, g1[var], tmp_out.str().c_str());
            tmp_out.str("");
#endif
            break;
          case Tags::FBINARY:
            op = static_cast<FBINARY_ *>(it_code->second)->get_op_type();
            v2 = Stack.top();
            Stack.pop();
            v1 = Stack.top();
            Stack.pop();
            if (compute)
              {
                v2f = Stackf.top();
                Stackf.pop();
                v1f = Stackf.top();
                Stackf.pop();
              }
            switch (static_cast<BinaryOpcode>(op))
              {
              case BinaryOpcode::plus:
#ifdef DEBUG
                mexPrintf("+");
#endif
                if (compute)
                  Stackf.push(v1f + v2f);
                tmp_out.str("");
                tmp_out << v1 << " + " << v2;
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::minus:
#ifdef DEBUG
                mexPrintf("-");
#endif
                if (compute)
                  Stackf.push(v1f - v2f);
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                tmp_out << " - ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::times:
#ifdef DEBUG
                mexPrintf("*");
#endif
                if (compute)
                  Stackf.push(v1f * v2f);
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                tmp_out << " * ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::divide:
#ifdef DEBUG
                mexPrintf("/");
#endif
                if (compute)
                  {
                    r = v1f / v2f;
                    Stackf.push(r);
                  }
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                if (compute)
                  {
                    if (isinf(r))
                      tmp_out << "$";
                    tmp_out << " / ";
                    if (isinf(r))
                      tmp_out << "£";
                  }
                else
                  tmp_out << " / ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::less:
#ifdef DEBUG
                mexPrintf("<");
#endif
                if (compute)
                  Stackf.push(static_cast<double>(v1f < v2f));
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                tmp_out << " < ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::greater:
#ifdef DEBUG
                mexPrintf(">");
#endif
                if (compute)
                  Stackf.push(static_cast<double>(v1f > v2f));
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                tmp_out << " > ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::lessEqual:
#ifdef DEBUG
                mexPrintf("<=");
#endif
                if (compute)
                  Stackf.push(static_cast<double>(v1f <= v2f));
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                tmp_out << " <= ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::greaterEqual:
#ifdef DEBUG
                mexPrintf(">=");
#endif
                if (compute)
                  Stackf.push(static_cast<double>(v1f >= v2f));
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                tmp_out << " >= ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::equalEqual:
#ifdef DEBUG
                mexPrintf("==");
#endif
                if (compute)
                  Stackf.push(static_cast<double>(v1f == v2f));
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                tmp_out << " == ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::different:
#ifdef DEBUG
                mexPrintf("!=");
#endif
                if (compute)
                  Stackf.push(static_cast<double>(v1f != v2f));
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                tmp_out << " != ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::power:
#ifdef DEBUG
                mexPrintf("^");
#endif
                if (compute)
                  {
                    r = pow(v1f, v2f);
                    Stackf.push(r);
                  }
                tmp_out.str("");
                found = v1.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v1;
                if (found != string::npos)
                  tmp_out << ")";
                if (compute)
                  {
                    if (isnan(r))
                      tmp_out << "$ ^ " << "£";
                    else
                      tmp_out << " ^ ";
                  }
                else
                  tmp_out << " ^ ";
                found = v2.find(" ");
                if (found != string::npos)
                  tmp_out << "(";
                tmp_out << v2;
                if (found != string::npos)
                  tmp_out << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::powerDeriv:
                {
                  v3 = Stack.top();
                  Stack.pop();
                  if (compute)
                    {
                      int derivOrder = static_cast<int>(nearbyint(Stackf.top()));
                      Stackf.pop();
                      if (fabs(v1f) < near_zero && v2f > 0
                          && derivOrder > v2f
                          && fabs(v2f-nearbyint(v2f)) < near_zero)
                        {
                          r = 0.0;
                          Stackf.push(r);
                        }
                      else
                        {
                          double dxp = pow(v1f, v2f-derivOrder);
                          for (int i = 0; i < derivOrder; i++)
                            dxp *= v2f--;
                          Stackf.push(dxp);
                          r = dxp;
                        }
                    }
                  tmp_out.str("");
                  if (compute)
                    {
                      if (isnan(r))
                        tmp_out << "$ PowerDeriv " << "£";
                      else
                        tmp_out << "PowerDeriv";
                    }
                  else
                    tmp_out << "PowerDeriv";
                  tmp_out << "(" << v1 << ", " << v2 << ", " << v3 << ")";
                  Stack.push(tmp_out.str());
                }
#ifdef DEBUG
                tmp_out << " |PowerDeriv(" << v1 << ", " << v2 << v3 << ")|";
#endif
                break;
              case BinaryOpcode::max:
#ifdef DEBUG
                mexPrintf("max");
#endif
                if (compute)
                  Stackf.push(max(v1f, v2f));
                tmp_out.str("");
                tmp_out << "max(" << v1 << ", " << v2 << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::min:
#ifdef DEBUG
                mexPrintf("min");
#endif
                if (compute)
                  Stackf.push(min(v1f, v2f));
                tmp_out.str("");
                tmp_out << "min(" << v1 << ", " << v2 << ")";
                Stack.push(tmp_out.str());
                break;
              case BinaryOpcode::equal:
              default:
                mexPrintf("Error unknown Unary operator=%d\n", op); mexEvalString("drawnow;");
                ;
              }
            break;
          case Tags::FUNARY:
            op = static_cast<FUNARY_ *>(it_code->second)->get_op_type();
            v1 = Stack.top();
            Stack.pop();
            if (compute)
              {
                v1f = Stackf.top();
                Stackf.pop();
              }
            switch (static_cast<UnaryOpcode>(op))
              {
              case UnaryOpcode::uminus:
                if (compute)
                  Stackf.push(-v1f);
                tmp_out.str("");
                tmp_out << " -" << v1;
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::exp:
                if (compute)
                  Stackf.push(exp(v1f));
                tmp_out.str("");
                tmp_out << "exp(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::log:
                if (compute)
                  {
                    r = log(v1f);
                    Stackf.push(r);
                  }
                tmp_out.str("");
                if (compute)
                  {
                    if (isnan(r))
                      tmp_out << "$log" << "£" << "(" << v1 << ")";
                    else
                      tmp_out << "log(" << v1 << ")";
                  }
                else
                  tmp_out << "log(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::log10:
                if (compute)
                  {
                    r = log10(v1f);
                    Stackf.push(r);
                  }
                tmp_out.str("");
                if (compute)
                  {
                    if (isnan(r))
                      tmp_out << "$log10" << "£" << "(" << v1 << ")";
                    else
                      tmp_out << "log10(" << v1 << ")";
                  }
                else
                  tmp_out << "log10(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::cos:
                if (compute)
                  Stackf.push(cos(v1f));
                tmp_out.str("");
                tmp_out << "cos(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::sin:
                if (compute)
                  Stackf.push(sin(v1f));
                tmp_out.str("");
                tmp_out << "sin(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::tan:
                if (compute)
                  Stackf.push(tan(v1f));
                tmp_out.str("");
                tmp_out << "tan(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::acos:
                if (compute)
                  Stackf.push(acos(v1f));
                tmp_out.str("");
                tmp_out << "acos(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::asin:
                if (compute)
                  Stackf.push(asin(v1f));
                tmp_out.str("");
                tmp_out << "asin(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::atan:
                if (compute)
                  Stackf.push(atan(v1f));
                tmp_out.str("");
                tmp_out << "atan(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::cosh:
                if (compute)
                  Stackf.push(cosh(v1f));
                tmp_out.str("");
                tmp_out << "cosh(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::sinh:
                if (compute)
                  Stackf.push(sinh(v1f));
                tmp_out.str("");
                tmp_out << "sinh(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::tanh:
                if (compute)
                  Stackf.push(tanh(v1f));
                tmp_out.str("");
                tmp_out << "tanh(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::acosh:
                if (compute)
                  Stackf.push(acosh(v1f));
                tmp_out.str("");
                tmp_out << "acosh(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::asinh:
                if (compute)
                  Stackf.push(asinh(v1f));
                tmp_out.str("");
                tmp_out << "asinh(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::atanh:
                if (compute)
                  Stackf.push(atanh(v1f));
                tmp_out.str("");
                tmp_out << "atanh(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::sqrt:
                if (compute)
                  Stackf.push(sqrt(v1f));
                tmp_out.str("");
                tmp_out << "sqrt(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              case UnaryOpcode::erf:
                if (compute)
                  Stackf.push(erf(v1f));
                tmp_out.str("");
                tmp_out << "erf(" << v1 << ")";
                Stack.push(tmp_out.str());
                break;
              default:
                mexPrintf("Error unknown Binary operator=%d\n", op); mexEvalString("drawnow;");
                ;
              }
            break;
          case Tags::FTRINARY:
            op = static_cast<FTRINARY_ *>(it_code->second)->get_op_type();
            v3 = Stack.top();
            Stack.pop();
            v2 = Stack.top();
            Stack.pop();
            v1 = Stack.top();
            Stack.pop();
            if (compute)
              {
                v3f = Stackf.top();
                Stackf.pop();
                v2f = Stackf.top();
                Stackf.pop();
                v1f = Stackf.top();
                Stackf.pop();
              }
            switch (static_cast<TrinaryOpcode>(op))
              {
              case TrinaryOpcode::normcdf:
                if (compute)
                  Stackf.push(0.5*(1+erf((v1f-v2f)/v3f/M_SQRT2)));
                tmp_out.str("");
                tmp_out << "normcdf(" << v1 << ", " << v2 << ", " << v3 << ")";
                Stack.push(tmp_out.str());
                break;
              case TrinaryOpcode::normpdf:
                if (compute)
                  Stackf.push(1/(v3f*sqrt(2*M_PI)*exp(pow((v1f-v2f)/v3f, 2)/2)));
                tmp_out.str("");
                tmp_out << "normpdf(" << v1 << ", " << v2 << ", " << v3 << ")";
                Stack.push(tmp_out.str());
                break;
              default:
                mexPrintf("Error unknown Trinary operator=%d\n", op); mexEvalString("drawnow;");
              }
            break;
          case Tags::FCALL:
            {
#ifdef DEBUG
              mexPrintf("------------------------------\n");
              mexPrintf("CALL "); mexEvalString("drawnow;");
#endif
              auto *fc = static_cast<FCALL_ *>(it_code->second);
              string function_name = fc->get_function_name();
#ifdef DEBUG
              mexPrintf("function_name=%s ", function_name.c_str()); mexEvalString("drawnow;");
#endif
              unsigned int nb_input_arguments = fc->get_nb_input_arguments();
#ifdef DEBUG
              mexPrintf("nb_input_arguments=%d ", nb_input_arguments); mexEvalString("drawnow;");
#endif
              unsigned int nb_output_arguments = fc->get_nb_output_arguments();
#ifdef DEBUG
              mexPrintf("nb_output_arguments=%d\n", nb_output_arguments); mexEvalString("drawnow;");
#endif

              mxArray *output_arguments[3];
              string arg_func_name = fc->get_arg_func_name();
#ifdef DEBUG
              mexPrintf("arg_func_name.length() = %d\n", arg_func_name.length());
              mexPrintf("arg_func_name.c_str() = %s\n", arg_func_name.c_str());
#endif
              unsigned int nb_add_input_arguments = fc->get_nb_add_input_arguments();
              function_type = fc->get_function_type();
#ifdef DEBUG
              mexPrintf("function_type=%d ExternalFunctionWithoutDerivative=%d\n", function_type, ExternalFunctionType::withoutDerivative);
              mexEvalString("drawnow;");
#endif
              mxArray **input_arguments;
              switch (function_type)
                {
                case ExternalFunctionType::withoutDerivative:
                case ExternalFunctionType::withFirstDerivative:
                case ExternalFunctionType::withFirstAndSecondDerivative:
                  {
                    if (compute)
                      {
                        input_arguments = static_cast<mxArray **>(mxMalloc(nb_input_arguments * sizeof(mxArray *)));
#ifdef DEBUG
                        mexPrintf("Stack.size()=%d\n", Stack.size());
                        mexEvalString("drawnow;");
#endif
                        for (unsigned int i = 0; i < nb_input_arguments; i++)
                          {
                            mxArray *vv = mxCreateDoubleScalar(Stackf.top());
                            input_arguments[nb_input_arguments - i - 1] = vv;
                            Stackf.pop();
                          }
                        mexCallMATLAB(nb_output_arguments, output_arguments, nb_input_arguments, input_arguments, function_name.c_str());
                        double *rr = mxGetPr(output_arguments[0]);
                        Stackf.push(*rr);
                      }
                    tmp_out.str("");
                    tmp_out << function_name << "(";
                    vector<string> ss(nb_input_arguments);
                    for (unsigned int i = 0; i < nb_input_arguments; i++)
                      {
                        ss[nb_input_arguments-i-1] = Stack.top();
                        Stack.pop();
                      }
                    for (unsigned int i = 0; i < nb_input_arguments; i++)
                      {
                        tmp_out << ss[i];
                        if (i < nb_input_arguments - 1)
                          tmp_out << ", ";
                      }
                    tmp_out << ")";
                    Stack.push(tmp_out.str());
                  }
                  break;
                case ExternalFunctionType::numericalFirstDerivative:
                  {
                    if (compute)
                      {
                        input_arguments = static_cast<mxArray **>(mxMalloc((nb_input_arguments+1+nb_add_input_arguments) * sizeof(mxArray *)));
                        mxArray *vv = mxCreateString(arg_func_name.c_str());
                        input_arguments[0] = vv;
                        vv = mxCreateDoubleScalar(fc->get_row());
                        input_arguments[1] = vv;
                        vv = mxCreateCellMatrix(1, nb_add_input_arguments);
                        for (unsigned int i = 0; i < nb_add_input_arguments; i++)
                          {
                            double rr = Stackf.top();
#ifdef DEBUG
                            mexPrintf("i=%d rr = %f Stack.size()=%d\n", i, rr, Stack.size());
#endif
                            mxSetCell(vv, nb_add_input_arguments - (i+1), mxCreateDoubleScalar(rr));
                            Stackf.pop();
                          }
                        input_arguments[nb_input_arguments+nb_add_input_arguments] = vv;
#ifdef DEBUG
                        mexCallMATLAB(0, nullptr, 1, &input_arguments[0], "disp");
                        mexCallMATLAB(0, nullptr, 1, &input_arguments[1], "disp");
                        mexCallMATLAB(0, nullptr, 1, &input_arguments[2], "celldisp");
                        mexPrintf("OK\n");
                        mexEvalString("drawnow;");
#endif
                        nb_input_arguments = 3;
                        mexCallMATLAB(nb_output_arguments, output_arguments, nb_input_arguments, input_arguments, function_name.c_str());
                        double *rr = mxGetPr(output_arguments[0]);
#ifdef DEBUG
                        mexPrintf("*rr=%f\n", *rr);
#endif
                        Stackf.push(*rr);
                      }
                    tmp_out.str("");
                    tmp_out << function_name << "(";
                    tmp_out << arg_func_name.c_str() << ", " << fc->get_row() << ", {";
                    vector<string> ss(nb_input_arguments);
                    for (unsigned int i = 0; i < nb_add_input_arguments; i++)
                      {
                        ss[nb_add_input_arguments-i-1] = Stack.top();
                        Stack.pop();
                      }
                    for (unsigned int i = 0; i < nb_add_input_arguments; i++)
                      {
                        tmp_out << ss[i];
                        if (i < nb_add_input_arguments - 1)
                          tmp_out << ", ";
                      }
                    tmp_out << "})";
                    Stack.push(tmp_out.str());
                  }
                  break;
                case ExternalFunctionType::firstDerivative:
                  {
                    if (compute)
                      {
                        input_arguments = static_cast<mxArray **>(mxMalloc(nb_input_arguments * sizeof(mxArray *)));
                        for (unsigned int i = 0; i < nb_input_arguments; i++)
                          {
                            mxArray *vv = mxCreateDoubleScalar(Stackf.top());
                            input_arguments[(nb_input_arguments - 1) - i] = vv;
                            Stackf.pop();
                          }
                        mexCallMATLAB(nb_output_arguments, output_arguments, nb_input_arguments, input_arguments, function_name.c_str());
                      }
                    tmp_out.str("");
                    tmp_out << function_name << "(";
                    vector<string> ss(nb_input_arguments);
                    for (unsigned int i = 0; i < nb_input_arguments; i++)
                      {
                        ss[nb_input_arguments-i-1] = Stack.top();
                        Stack.pop();
                      }
                    for (unsigned int i = 0; i < nb_input_arguments; i++)
                      {
                        tmp_out << ss[i];
                        if (i < nb_input_arguments - 1)
                          tmp_out << ", ";
                      }
                    tmp_out << ")";
                    Stack.push(tmp_out.str());
                  }
                  break;
                case ExternalFunctionType::numericalSecondDerivative:
                  {
                    if (compute)
                      {
                        input_arguments = static_cast<mxArray **>(mxMalloc((nb_input_arguments+1+nb_add_input_arguments) * sizeof(mxArray *)));
                        mxArray *vv = mxCreateString(arg_func_name.c_str());
                        input_arguments[0] = vv;
                        vv = mxCreateDoubleScalar(fc->get_row());
                        input_arguments[1] = vv;
                        vv = mxCreateDoubleScalar(fc->get_col());
                        input_arguments[2] = vv;
                        vv = mxCreateCellMatrix(1, nb_add_input_arguments);
                        for (unsigned int i = 0; i < nb_add_input_arguments; i++)
                          {
                            double rr = Stackf.top();
#ifdef DEBUG
                            mexPrintf("i=%d rr = %f\n", i, rr);
#endif
                            mxSetCell(vv, (nb_add_input_arguments - 1) - i, mxCreateDoubleScalar(rr));
                            Stackf.pop();
                          }
                        input_arguments[nb_input_arguments+nb_add_input_arguments] = vv;
#ifdef DEBUG
                        mexCallMATLAB(0, nullptr, 1, &input_arguments[0], "disp");
                        mexCallMATLAB(0, nullptr, 1, &input_arguments[1], "disp");
                        mexCallMATLAB(0, nullptr, 1, &input_arguments[2], "celldisp");
                        mexPrintf("OK\n");
                        mexEvalString("drawnow;");
#endif
                        nb_input_arguments = 3;
                        mexCallMATLAB(nb_output_arguments, output_arguments, nb_input_arguments, input_arguments, function_name.c_str());
                        double *rr = mxGetPr(output_arguments[0]);
                        Stackf.push(*rr);
                      }
                    tmp_out.str("");
                    tmp_out << function_name << "(";
                    tmp_out << arg_func_name.c_str() << ", " << fc->get_row() << ", " << fc->get_col() << ", {";
                    vector<string> ss(nb_input_arguments);
                    for (unsigned int i = 0; i < nb_add_input_arguments; i++)
                      {
                        ss[nb_add_input_arguments-i-1] = Stack.top();
                        Stack.pop();
                      }
                    for (unsigned int i = 0; i < nb_add_input_arguments; i++)
                      {
                        tmp_out << ss[i];
                        if (i < nb_add_input_arguments - 1)
                          tmp_out << ", ";
                      }
                    tmp_out << "})";
                    Stack.push(tmp_out.str());
                  }
                  break;
                case ExternalFunctionType::secondDerivative:
                  {
                    if (compute)
                      {
                        input_arguments = static_cast<mxArray **>(mxMalloc(nb_input_arguments * sizeof(mxArray *)));
                        for (unsigned int i = 0; i < nb_input_arguments; i++)
                          {
                            mxArray *vv = mxCreateDoubleScalar(Stackf.top());
                            input_arguments[i] = vv;
                            Stackf.pop();
                          }
                        mexCallMATLAB(nb_output_arguments, output_arguments, nb_input_arguments, input_arguments, function_name.c_str());
                      }
                    tmp_out.str("");
                    tmp_out << function_name << "(";
                    vector<string> ss(nb_input_arguments);
                    for (unsigned int i = 0; i < nb_input_arguments; i++)
                      {
                        ss[nb_input_arguments-i-1] = Stack.top();
                        Stack.pop();
                      }
                    for (unsigned int i = 0; i < nb_input_arguments; i++)
                      {
                        tmp_out << ss[i];
                        if (i < nb_input_arguments - 1)
                          tmp_out << ", ";
                      }
                    tmp_out << ")";
                    Stack.push(tmp_out.str());
                  }
                  break;
                }
#ifdef DEBUG
              mexPrintf("end CALL\n");
              mexEvalString("drawnow;");
#endif
              break;
            }
          case Tags::FSTPTEF:
            go_on = false;
            var = static_cast<FSTPTEF_ *>(it_code->second)->get_number();
#ifdef DEBUG
            mexPrintf("FSTPTEF\n");
            mexPrintf("var=%d Stack.size()=%d\n", var, Stack.size());
#endif
            if (compute)
              {
#ifdef DEBUG
                double rr = Stackf.top();
                mexPrintf("FSTP TEF(var-1)=%f done\n", rr);
                mexEvalString("drawnow;");
#endif
                Stackf.pop();
              }
            tmp_out.str("");
            switch (function_type)
              {
              case ExternalFunctionType::withoutDerivative:
                tmp_out << "TEF(" << var << ") = " << Stack.top();
                break;
              case ExternalFunctionType::withFirstDerivative:
                tmp_out << "[TEF(" << var << "), TEFD(" << var << ") ]= " << Stack.top();
                break;
              case ExternalFunctionType::withFirstAndSecondDerivative:
                tmp_out << "[TEF(" << var << "), TEFD(" << var << "), TEFDD(" << var << ") ]= " << Stack.top();
                break;
              default:
                break;
              }
            Stack.pop();
#ifdef DEBUG
            mexPrintf("end FSTPEF\n");
            mexEvalString("drawnow;");
#endif
            break;
          case Tags::FLDTEF:
            var = static_cast<FLDTEF_ *>(it_code->second)->get_number();
#ifdef DEBUG
            mexPrintf("FLDTEF\n");
            mexPrintf("var=%d Stack.size()=%d\n", var, Stackf.size());
            {
              auto it = TEF.find(var-1);
              mexPrintf("FLD TEF[var-1]=%f done\n", it->second);
            }
            mexEvalString("drawnow;");
#endif
            if (compute)
              {
                auto it = TEF.find(var-1);
                Stackf.push(it->second);
              }
            tmp_out.str("");
            tmp_out << "TEF(" << var << ")";
            Stack.push(tmp_out.str());
#ifdef DEBUG
            mexPrintf("end FLDTEF\n");
            mexEvalString("drawnow;");
#endif

            break;
          case Tags::FSTPTEFD:
            {
              go_on = false;
              unsigned int indx = static_cast<FSTPTEFD_ *>(it_code->second)->get_indx();
              unsigned int row = static_cast<FSTPTEFD_ *>(it_code->second)->get_row();
#ifdef DEBUG
              mexPrintf("FSTPTEFD\n");
              mexPrintf("indx=%d Stack.size()=%d\n", indx, Stack.size());
#endif
              if (compute)
                {
#ifdef DEBUG
                  double rr = Stackf.top();
                  mexPrintf("FSTP TEFD[{ indx, row }]=%f done\n", rr);
                  mexEvalString("drawnow;");
#endif
                  Stackf.pop();
                }
              tmp_out.str("");
              if (function_type == ExternalFunctionType::numericalFirstDerivative)
                tmp_out << "TEFD(" << indx << ", " << row << ") = " << Stack.top();
              else if (function_type == ExternalFunctionType::firstDerivative)
                tmp_out << "TEFD(" << indx << ") = " << Stack.top();
              Stack.pop();
            }
            break;
          case Tags::FLDTEFD:
            {
              unsigned int indx = static_cast<FLDTEFD_ *>(it_code->second)->get_indx();
              unsigned int row = static_cast<FLDTEFD_ *>(it_code->second)->get_row();
#ifdef DEBUG
              mexPrintf("FLDTEFD\n");
              mexPrintf("indx=%d row=%d Stack.size()=%d\n", indx, row, Stack.size());
              auto it = TEFD.find({ indx, row-1 });
              mexPrintf("FLD TEFD[{ indx, row }]=%f done\n", it->second);
              mexEvalString("drawnow;");
#endif
              if (compute)
                {
                  auto it = TEFD.find({ indx, row-1 });
                  Stackf.push(it->second);
                }
              tmp_out.str("");
              tmp_out << "TEFD(" << indx << ", " << row << ")";
              Stack.push(tmp_out.str());
            }
            break;
          case Tags::FSTPTEFDD:
            {
              go_on = false;
              unsigned int indx = static_cast<FSTPTEFDD_ *>(it_code->second)->get_indx();
              unsigned int row = static_cast<FSTPTEFDD_ *>(it_code->second)->get_row();
              unsigned int col = static_cast<FSTPTEFDD_ *>(it_code->second)->get_col();
#ifdef DEBUG
              mexPrintf("FSTPTEFD\n");
              mexPrintf("indx=%d Stack.size()=%d\n", indx, Stack.size());
#endif
              if (compute)
                {
#ifdef DEBUG
                  double rr = Stackf.top();
                  mexPrintf("rr=%f\n", rr);
                  auto it = TEFDD.find({ indx, row-1, col-1 });
                  mexPrintf("FSTP TEFDD[{ indx, row, col }]=%f done\n", it->second);
                  mexEvalString("drawnow;");
#endif
                  Stackf.pop();
                }
              tmp_out.str("");
              if (function_type == ExternalFunctionType::numericalSecondDerivative)
                tmp_out << "TEFDD(" << indx << ", " << row << ", " << col << ") = " << Stack.top();
              else if (function_type == ExternalFunctionType::secondDerivative)
                tmp_out << "TEFDD(" << indx << ") = " << Stack.top();
              Stack.pop();
            }

            break;
          case Tags::FLDTEFDD:
            {
              unsigned int indx = static_cast<FLDTEFDD_ *>(it_code->second)->get_indx();
              unsigned int row = static_cast<FLDTEFDD_ *>(it_code->second)->get_row();
              unsigned int col = static_cast<FSTPTEFDD_ *>(it_code->second)->get_col();
#ifdef DEBUG
              mexPrintf("FLDTEFD\n");
              mexPrintf("indx=%d Stack.size()=%d\n", indx, Stack.size());
              auto it = TEFDD.find({ indx, row-1, col-1 });
              mexPrintf("FLD TEFD[{ indx, row, col }]=%f done\n", it->second);
              mexEvalString("drawnow;");
#endif
              if (compute)
                {
                  auto it = TEFDD.find({ indx, row-1, col-1 });
                  Stackf.push(it->second);
                }
              tmp_out.str("");
              tmp_out << "TEFDD(" << indx << ", " << row << ", " << col << ")";
              Stack.push(tmp_out.str());
            }
            break;
          case Tags::FJMPIFEVAL:
            tmp_out.str("");
            tmp_out << "if (~evaluate)";
            go_on = false;
            break;
          case Tags::FJMP:
            tmp_out.str("");
            tmp_out << "else";
            go_on = false;
            break;
          case Tags::FCUML:
            if (compute)
              {
                v1f = Stackf.top();
                Stackf.pop();
                v2f = Stackf.top();
                Stackf.pop();
                Stackf.push(v1f+v2f);
              }
            v1 = Stack.top();
            Stack.pop();
            v2 = Stack.top();
            Stack.pop();
            tmp_out.str("");
            tmp_out << v1 << " + " << v2;
            Stack.push(tmp_out.str());
            break;
          case Tags::FENDBLOCK:
          case Tags::FENDEQU:
            go_on = false;
            break;
          case Tags::FOK:
            break;
          default:
            mexPrintf("Error it_code->first=%d unknown\n", it_code->first); mexEvalString("drawnow;");
            throw FatalExceptionHandling(" in print_expression, unknown opcode "
                                         + to_string(static_cast<int>(it_code->first))
                                         + "!! FENDEQU="
                                         + to_string(static_cast<int>(Tags::FENDEQU)) + "\n");
          }
        it_code++;
      }
#ifdef DEBUG
    mexPrintf("print_expression end tmp_out.str().c_str()=%s\n", tmp_out.str().c_str()); mexEvalString("drawnow;");
#endif
    it_code_ret = it_code;
    return tmp_out.str();
  }

  static inline void
  test_mxMalloc(void *z, int line, const string &file, const string &func, int amount)
  {
    if (!z && amount > 0)
      throw FatalExceptionHandling(" mxMalloc: out of memory " + to_string(amount) + " bytes required at line " + to_string(line) + " in function " + func + " (file " + file);
  }
};

#endif
