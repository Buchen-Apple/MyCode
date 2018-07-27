#include <stdio.h>
#include "Binary_Search_Tree.h"

int main()
{
	/*CBST cBinaryTree(10);
	cBinaryTree.Insert(9);
	cBinaryTree.Insert(11);

	cBinaryTree.Delete(10);

	printf("루트노드 : %d\n", cBinaryTree.GetRootNodeData());
	cBinaryTree.Traversal();*/


	CBST cBinaryTree(10);
	printf("루트노드 : %d\n", cBinaryTree.GetRootNodeData());

	cBinaryTree.Insert(14);
	cBinaryTree.Insert(12);
	cBinaryTree.Insert(11);
	cBinaryTree.Insert(13);
	
	cBinaryTree.Insert(8);
	cBinaryTree.Insert(9);
	cBinaryTree.Insert(5);
	cBinaryTree.Insert(6);
	cBinaryTree.Insert(7);

	cBinaryTree.Traversal();
	fputc('\n', stdout);

	// 루트노드(10) 삭제
	printf("\n%d 삭제!\n", cBinaryTree.GetRootNodeData());
	cBinaryTree.Delete(cBinaryTree.GetRootNodeData());

	printf("루트노드 : %d\n", cBinaryTree.GetRootNodeData());
	cBinaryTree.Traversal();
	fputc('\n', stdout);	

	// 7, 8 삭제
	printf("\n%d,%d 삭제!\n", 7, 8);
	cBinaryTree.Delete(7);
	cBinaryTree.Delete(8);

	printf("루트노드 : %d\n", cBinaryTree.GetRootNodeData());
	cBinaryTree.Traversal();
	fputc('\n', stdout);






	//// 12, 13 삭제
	//printf("\n%d, %d 삭제!\n", 12, 13);
	//cBinaryTree.Delete(13);
	//cBinaryTree.Delete(12);

	//printf("루트노드 : %d\n", cBinaryTree.GetRootNodeData());
	//cBinaryTree.Traversal();
	//fputc('\n', stdout);

	//// 루트노드(9) 삭제
	//printf("\n%d 삭제!\n", cBinaryTree.GetRootNodeData());
	//cBinaryTree.Delete(cBinaryTree.GetRootNodeData());

	//printf("루트노드 : %d\n", cBinaryTree.GetRootNodeData());
	//cBinaryTree.Traversal();
	//fputc('\n', stdout);

	//// 5 삭제
	//printf("\n%d 삭제!\n", 5);
	//cBinaryTree.Delete(5);

	//printf("루트노드 : %d\n", cBinaryTree.GetRootNodeData());
	//cBinaryTree.Traversal();
	//fputc('\n', stdout);

	//// 루트노드(6) 삭제
	//printf("\n%d 삭제!\n", cBinaryTree.GetRootNodeData());
	//cBinaryTree.Delete(cBinaryTree.GetRootNodeData());

	//printf("루트노드 : %d\n", cBinaryTree.GetRootNodeData());
	//cBinaryTree.Traversal();
	//fputc('\n', stdout);



	return 0;
}