/*
 * Command DFA module.
 * Provides a DFA data structure and associated functions for manipulating it.
 * Used to match user command line input.
 *
 * @author Quentin Young <qlyoung@cumulusnetworks.com>
 */

#include "command_graph.h"
#include <zebra.h>
#include "memory.h"

struct graph_node *
add_node(struct graph_node *parent, struct graph_node *child)
{
  struct graph_node *p_child;

  for (unsigned int i = 0; i < vector_active(parent->children); i++)
  {
    p_child = vector_slot(parent->children, i);
    if (cmp_node(child, p_child))
      return p_child;
  }
  vector_set(parent->children, child);
  return child;
}

int
cmp_node(struct graph_node *first, struct graph_node *second)
{
  // compare types
  if (first->type != second->type) return 0;

  switch (first->type) {
    case WORD_GN:
    case VARIABLE_GN:
      if (first->text && second->text) {
        if (strcmp(first->text, second->text)) return 0;
      }
      else if (first->text != second->text) return 0;
      break;
    case RANGE_GN:
      if (first->min != second->min || first->max != second->max)
        return 0;
      break;
    case NUMBER_GN:
      if (first->value != second->value) return 0;
      break;
    /* selectors and options should be equal if all paths are equal,
     * but the graph isomorphism problem is not solvable in polynomial
     * time so we consider selectors and options inequal in all cases
     */
    case SELECTOR_GN:
    case OPTION_GN:
      return 0;
    /* end nodes are always considered equal, since each node may only
     * have one at a time
     */
    case START_GN:
    case END_GN:
    default:
      break;
  }

  return 1;
}

struct graph_node *
new_node(enum graph_node_type type)
{
  struct graph_node *node =
     XMALLOC(MTYPE_CMD_TOKENS, sizeof(struct graph_node));

  node->type = type;
  node->children = vector_init(VECTOR_MIN_SIZE);
  node->is_start = 0;
  node->end      = NULL;
  node->text     = NULL;
  node->value    = 0;
  node->min      = 0;
  node->max      = 0;
  node->element  = NULL;

  return node;
}

void
free_node (struct graph_node *node)
{
  if (!node) return;
  free_node (node->end);
  vector_free (node->children);
  free (node->text);
  free (node->arg);
  free (node->element);
  free (node);
}

char *
describe_node(struct graph_node *node, char* buffer, unsigned int bufsize)
{
  if (node == NULL) {
    snprintf(buffer, bufsize, "(null node)");
    return buffer;
  }

  // print this node
  switch (node->type) {
    case WORD_GN:
    case IPV4_GN:
    case IPV4_PREFIX_GN:
    case IPV6_GN:
    case IPV6_PREFIX_GN:
    case VARIABLE_GN:
    case RANGE_GN:
      snprintf(buffer, bufsize, node->text);
      break;
    case NUMBER_GN:
      snprintf(buffer, bufsize, "%ld", node->value);
      break;
    case SELECTOR_GN:
      snprintf(buffer, bufsize, "<>");
      break;
    case OPTION_GN:
      snprintf(buffer, bufsize, "[]");
      break;
    case NUL_GN:
      snprintf(buffer, bufsize, "NUL");
      break;
    case END_GN:
      snprintf(buffer, bufsize, "END");
      break;
    case START_GN:
      snprintf(buffer, bufsize, "START");
      break;
    default:
      snprintf(buffer, bufsize, "ERROR");
  }

  return buffer;
}


void
walk_graph(struct graph_node *start, int level)
{
  char* desc = malloc(50);
  // print this node
  fprintf(stderr, "%s[%d] ", describe_node(start, desc, 50), vector_active(start->children));
  free(desc);

  if (vector_active(start->children)) {
    if (vector_active(start->children) == 1)
      walk_graph(vector_slot(start->children, 0), level);
    else {
      fprintf(stderr, "\n");
      for (unsigned int i = 0; i < vector_active(start->children); i++) {
        struct graph_node *r = vector_slot(start->children, i);
        for (int j = 0; j < level+1; j++)
          fprintf(stderr, "    ");
        walk_graph(r, level+1);
      }
    }
  }
  else
    fprintf(stderr, "\n");
}

