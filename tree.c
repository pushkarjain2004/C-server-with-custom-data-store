#include "tree.h"
// tree.c (at global scope, after includes)
Nullptr my_null = 0; // <-- Add this definition here
Tree root={.n={
    .tag=(TagRoot | TagNode),
    .north=(Node*) &root,
    .west=0,
    .east=0,
    .path= "/"
}};

void print_tree_forward_leaves(int fd, Tree * _root){
    int8 indentation;
    int8 buf[256]; // Buffer for the Print macro
    int16 size;    // Size for the Print macro
    Node *n;       // Pointer for Node traversal
    Leaf *l;       // Pointer for Leaf traversal

    indentation = 0; 

    // Outer loop: Traverse down the 'west' branches of the Node tree (e.g., / -> /Users -> /Users/login)
    for(n = (Node*)_root; n != NULL; n = n->west){
        // Print the Node itself
        Print(indent(indentation)); // Indent the Node
        Print(n->path);             // Print the Node's path
        Print("\n");                // Newline after the Node path

        // Increment indentation for children/leaves under this Node
        indentation++;

        // Inner loop: Traverse 'east' chain to print all Leaves associated with this Node
        if(n->east){ // Check if this Node has any leaves
            for(l = n->east; l != NULL; l = l->east){ // Start from the first leaf (n->east) and follow 'east' pointers
                Print(indent(indentation)); // Indent leaves deeper than their parent Node
                Print(n->path);             // Print Node's path (e.g., /Users/login)
                Print("/");
                Print(l->key);              // Print the Leaf's key (e.g., manan)
                Print(" ->'");
                // Directly write the value, as Print macro is for string literals/char arrays
                write(fd, (char *)l->value, (int)l->size); // Write the raw value data
                Print("'\n"); // Newline after the Leaf entry
            }
        }
    }
    
    return;
}
int8 *indent(int8 n){
   int16 i;
   static int8 buf[256];
   int8 *p;
   if(n<1)
        return (int8 *)"";

   assert(n<120);
   zero(buf,256);
   for(i=0,p=buf;i<n;i++,p+=2)
        strncpy((char *)p,"  ",2);

    return buf;
}

/*
struct s_node {
    Tag tag;
    struct s_node *north;
    struct s_node *west;
    struct s_leaf *east;
    int8 path[256];
}; 
*/
void zero(int8 *str,int16 size){
    int8 *p;
    int16 n;
    for(n=0,p=str;n<size;p++,n++)
         *p=0;
    return ;   
}
Node *create_node(Node* parent,int8 *path){
    Node *n;
    int16 size;
    errno=NoError;
    assert(parent);
    size=sizeof(struct s_node);
    n=(Node*)malloc((int )size);
    zero((int8 *)n,size);
    parent->west=n;
    n->tag=TagNode;
    n->north=parent;
    strncpy((char *)n->path,(char *)path,255);
    return n;
} 
Node *find_node_linear(int8 *path){
    Node *p,*ret;
    for(ret =(Node *)0,p=(Node *)&root;p;p=p->west){
        if(!strcmp((char *)p->path,(char *)path)){
            ret=p;
            break;
        }
    }
    return ret;
}
Leaf *find_leaf_linear(int8 *path,int8 *key){
    Node *n;
    Leaf *l,*ret;
    n=find_node(path);
    if(!n)
        return (Leaf *)0;
    
    for(ret =(Leaf *)0,l=n->east;l;l=l->east)
        if(!strcmp((char *)l->key,(char *)key)){
            ret=l;
            break;
        }
return ret;
}
int8 *lookup_linear(int8 *path,int8 *key){
    Leaf *p;
    p=find_leaf(path,key);
    return (p) ?
        p->value :
    (int8 *)0;

}

Leaf *find_last_linear(Node* parent){
    Leaf *l;
    errno=NoError;
    assert(parent);
    if(!parent->east)
            return (Leaf *)0;

    
    for(l=parent->east;l->east;l=l->east);
    assert(l);
    return l;
    
}
Leaf *create_leaf(Node *parent,int8 *key,int8 *value,int16 count){
    Leaf *l,*new;
    int16 size;
    assert(parent);
    l=find_last(parent);
    size=sizeof(struct s_leaf);
    new=(Leaf * )malloc(size);
    assert(new);
    if(!l)
        //direct connected
        parent->east=new;
    else    
        // l is a leaf
        l->east=new;
    zero((int8 *)new ,size);
    new->tag=TagLeaf;
    new->west=(!l) ?
        (Tree *)parent:
    (Tree *)l;

    strncpy((char *)new->key,(char *)key,127);
    new->value=(int8 *)malloc(count);
    zero(new->value,count);
    assert(new->value);
    strncpy((char *)new->value,(char * )value,count);
    new->size=count;
    return new;
}
int tree_test_main(){
   Node* n,*n2;
   Leaf *l1,*l2;
   int8 *key,*value;
   int16 size;
   int8 *test;


    n=create_node((Node *)&root, (int8*)"/Users");
    assert(n);
    n2=create_node(n,(int8*)"/Users/login");
    assert(n2);
    key=(int8*)"pushkar";
    value=(int8 *)"abs77301aa";
    size=(int16)strlen((char*)value);
    l1=create_leaf(n2,key,value,size);
    assert(l1);
    //printf("%s\n",l1->value);
    key=(int8 *)"manan";
    value=(int8*)"aa098765467c";
    size=(int16)strlen((char*)value);
    l2=create_leaf(n2,key,value,size);
    assert(l2);
    //printf("%s\n",l2->key);
    //printf("%p %p \n",n,n2);
    
    test=lookup((int8 *)"/Users/login",(int8 *)"pushkar");
    if(test)
        printf("%s\n",test);
    else
        printf("No\n");
    //printf("%p\n",find_node_linear((int8 *)"/Users/login"));
    free(n2);
    free(n);
    return 0;
}