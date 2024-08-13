#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpc.h"

/* If we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* 'Fake' readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* 'Fake' add_history function */ 
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else 
#include <editline/readline.h>
#include <histedit.h>
#endif

/* Declare New lval Struct */
typedef struct {
  int type;
  double num;
  int err;
} lval;

/* Enumeration of Possible lval Types */
enum { LVAL_NUM, LVAL_ERR };

/* Enumeration of Possible Error Types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Create a new number type lval */
lval lval_num(double x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

/* Create a new error type lval */
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

/* Print an "lval" */
void lval_print(lval v) {
  switch (v.type) {
    /* In the case the type is a number print it */
    /* Then 'break' out of the switch. */
    case LVAL_NUM: printf("%f", v.num); break;

    /* In the case the type is an error */
    case LVAL_ERR:
      /* Check what type of error it is and print it */
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division By Zero!");
      }
      if (v.err == LERR_BAD_OP) {
        printf("Error: Invalid Operator!");
      }  
      if (v.err == LERR_BAD_NUM) {
        printf("Error: Invalid Number!");
      }
    break;
  }
}

/* Print an "lval" followed by a newline */
void lval_println(lval v) { lval_print(v); putchar('\n'); }

lval eval_op(lval x, char* op, lval y) {
  /* If either value is an error return it */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num);             }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num);             }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num);             }
  if (strcmp(op, "/") == 0) { 
    /* If second operand is zero return error */
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }
  if (strcmp(op, "%") == 0) { return lval_num(fmod(x.num, y.num));             }
  if (strcmp(op, "^") == 0) { return lval_num(pow(x.num, y.num));         }

  if (strcmp(op, "add") == 0) { return lval_num(x.num + y.num);           }
  if (strcmp(op, "sub") == 0) { return lval_num(x.num - y.num);           }
  if (strcmp(op, "mul") == 0) { return lval_num(x.num * y.num);           }
  if (strcmp(op, "div") == 0) { 
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }
  if (strcmp(op, "mod") == 0) { return lval_num(fmod(x.num, y.num));  }
  if (strcmp(op, "min") == 0) { return lval_num(fmin(x.num, y.num));  }
  if (strcmp(op, "max") == 0) { return lval_num(fmax(x.num, y.num));  }
  
  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
  
  if (strstr(t->tag, "number")) {
    /* Check if there is some error in conversion */
    errno = 0;
    double x = strtod(t->contents, NULL);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  char* op = t->children[1]->contents;
  lval x = eval(t->children[2]);

  /* If the operator is "min", iterate through all children to find the minimum */
  if (strcmp(op, "min") == 0) {
    lval min_value = x; // Initialize min_value to the first operand
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
      lval y = eval(t->children[i]);
      if (y.num < min_value.num) {
        min_value.num = y.num; // Update min_value if y is smaller
      }
      i++;
    }
    return min_value; // Return the smallest number
  }

  /* If the operator is "max" */
  if (strcmp(op, "max") == 0) {
    lval max_value = x;
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
      lval y = eval(t->children[i]);
      if (y.num > max_value.num) {
        max_value = y;
      }
      i++;
    }
    return max_value;
  }

  /* Iterate the remaining children and combine. */
  int i = 3;
  while(strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char** argv) {

  /* Create Some Parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Styx     = mpc_new("styx");

  /* Define them with the following Language */
   
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                              \
      number   : /-?[0-9]+\\.?[0-9]*/ ;                            \
      operator : '+' | '-' | '*' | '/' | '%' | '^' | \"add\" | \"sub\" | \"mul\" | \"div\" | \"mod\" | \"min\" | \"max\";  \
      expr     : <number> | '(' <operator> <expr>+ ')' ;           \
      styx     : /^/ <operator> <expr>+ /$/ ;                      \
    ",
    Number, Operator, Expr, Styx);

  puts("Styx Version 0.0.0.0.3");
  puts("Press Ctrl+c to Exit\n");

  while(1) {
    
    char* input = readline("Styx> ");
    add_history(input);

    /* Attempt to Parse the user Input */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Styx, &r)) {
      /* On Success evaluate the AST */
      lval result = eval(r.output);
      lval_println(result);
      mpc_ast_delete(r.output);
    } else {
      /* Otherwise Print the Error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  /* Undefine and Delete our Parsers */
  mpc_cleanup(4, Number, Operator, Expr, Styx);

  return 0;
}
