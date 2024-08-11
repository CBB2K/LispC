#include <stdio.h>
#include <stdlib.h>
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

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y;             }
  if (strcmp(op, "-") == 0) { return x - y;             }
  if (strcmp(op, "*") == 0) { return x * y;             }
  if (strcmp(op, "/") == 0) { return x / y;             }
  if (strcmp(op, "%") == 0) { return x % y;             }
  if (strcmp(op, "^") == 0) { return pow(x, y);         }

  if (strcmp(op, "add") == 0) { return x + y;           }
  if (strcmp(op, "sub") == 0) { return x - y;           }
  if (strcmp(op, "mul") == 0) { return x * y;           }
  if (strcmp(op, "div") == 0) { return x / y;           }
  if (strcmp(op, "mod") == 0) { return x % y;           }
  if (strcmp(op, "min") == 0) { return (x < y) ? x : y; }
  if (strcmp(op, "max") == 0) { return (x > y) ? x : y; }
  
  return 0;
}

long eval(mpc_ast_t* t) {
  /* If tagged as number return it directly. */
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  /* The operator is always the second child. */
  char* op = t->children[1]->contents;

  /* Store the next child in 'x'. */
  long x = eval(t->children[2]);

  /* If the operator is "min", iterate through all children to find the minimum */
  if (strcmp(op, "min") == 0) {
    long min_value = x; // Initialize min_value to the first operand
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
      long y = eval(t->children[i]);
      if (y < min_value) {
        min_value = y; // Update min_value if y is smaller
      }
      i++;
    }
    return min_value; // Return the smallest number
  }

  /* If the operator is "max" */
  if (strcmp(op, "max") == 0) {
    long max_value = x;
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
      long y = eval(t->children[i]);
      if (y > max_value) {
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
      long result = eval(r.output);
      printf("%li\n", result);
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
