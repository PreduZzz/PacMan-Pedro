#include "pacman.h"
int manhattan(int x1, int y1, int x2, int y2) { return abs(x1-x2)+abs(y1-y2); }

static int is_desc(int *v, int n) {
    for (int i = 1; i < n; i++) if (v[i-1] < v[i]) return 0;
    return 1;
}
static void cp(int *dst, const int *src, int n){ for(int i=0;i<n;i++) dst[i]=src[i]; }

int main(void) {
    int pass = 1;
    BSTNode *bst = NULL;
    bst = bst_insert(bst, 50, "Harry");
    bst = bst_insert(bst, 20, "Ron");
    bst = bst_insert(bst, 70, "Hermione");
    bst = bst_insert(bst, 10, "Neville");
    bst = bst_insert(bst, 30, "Luna");
    if (!bst_search(bst, 30) || bst_height(bst) != 3 || bst_count(bst) != 5) pass = 0;
    bst = bst_remove(bst, 10); /* folha */
    bst = bst_remove(bst, 20); /* um filho */
    bst = bst_remove(bst, 50); /* dois filhos */
    if (bst_search(bst, 10) || bst_count(bst) != 2) pass = 0;
    bst_free(bst);
    printf("BST: %s\n", pass ? "PASS" : "FAIL");

    AVLNode *avl = NULL;
    int keys[] = {30,20,10,40,50,25};
    for (int i=0;i<6;i++) avl = avl_insert(avl, keys[i], 1);
    int bf = avl_balance_factor(avl);
    int avl_ok = (bf >= -1 && bf <= 1 && avl_search(avl, 25));
    printf("AVL: %s\n", avl_ok ? "PASS" : "FAIL");
    pass = pass && avl_ok;
    avl_free(avl);

    int base[] = {5,1,9,3,7,2};
    int a[6];
    cp(a,base,6); bubble_sort(a,6);    printf("Bubble: %s\n", is_desc(a,6)?"PASS":"FAIL"); pass &= is_desc(a,6);
    cp(a,base,6); selection_sort(a,6); printf("Selection: %s\n", is_desc(a,6)?"PASS":"FAIL"); pass &= is_desc(a,6);
    cp(a,base,6); insertion_sort(a,6); printf("Insertion: %s\n", is_desc(a,6)?"PASS":"FAIL"); pass &= is_desc(a,6);
    cp(a,base,6); shell_sort(a,6);     printf("Shell: %s\n", is_desc(a,6)?"PASS":"FAIL"); pass &= is_desc(a,6);
    cp(a,base,6); merge_sort(a,6);     printf("Merge: %s\n", is_desc(a,6)?"PASS":"FAIL"); pass &= is_desc(a,6);
    cp(a,base,6); quick_sort(a,0,5);   printf("Quick: %s\n", is_desc(a,6)?"PASS":"FAIL"); pass &= is_desc(a,6);
    cp(a,base,6); heap_sort(a,6);      printf("Heap: %s\n", is_desc(a,6)?"PASS":"FAIL"); pass &= is_desc(a,6);

    return pass ? 0 : 1;
}
