#include "AVL_Tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main()
{
	CAVLTree avlTree;
	BTreeNode* root = avlTree.GetRootNode();

	srand(time(NULL));

	for (int i = 0; i < 20; ++i)
	{		
		int a = rand() % 20 + 1;
		avlTree.Insert(&root, a);

		root = avlTree.GetRootNode();
		printf("»ðÀÔ : %d, Root : %d\n", a, avlTree.GetData(root));

		
	}

	root = avlTree.GetRootNode();
	avlTree.InorderTraversal(root);

	printf("\n\n\n");

	for (int i = 0; i < 20; ++i)
	{
		int a = rand() % 20 + 1;
		avlTree.Delete(a);

		root = avlTree.GetRootNode();;
		printf("»èÁ¦ : %d, Root : %d\n", a, avlTree.GetData(root));	
	}

	root = avlTree.GetRootNode();
	avlTree.InorderTraversal(root);


	return 0;
}