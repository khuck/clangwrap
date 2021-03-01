/****************************************************************************
 **  TAU Portable Profiling Package                                        **
 **  http://tau.uoregon.edu                                                **
 ****************************************************************************
 **  Copyright 2021                                                        **
 **  Department of Computer and Information Science, University of Oregon  **
 ****************************************************************************/

// CPP program to implement sequence alignment
// problem.
//#include <bits/stdc++.h>
// Found at https://www.geeksforgeeks.org/sequence-alignment-problem/
// Fixed by khuck
#include <string>
#include <iostream>
#include <algorithm>

// function to find out the minimum penalty
int getMinimumPenalty(std::string x, std::string y, int pxy, int pgap, bool report = false)
{
	int i, j; // intialising variables

	int m = x.length(); // length of gene1
	int n = y.length(); // length of gene2

	// table for storing optimal substructure answers
	//int * dp = new int[m+1][n+1] = {0};
	int ** dp = new int*[m+1];
    for (i = 0 ; i < m+1 ; i++) {
        dp[i] = new int[n+1];
        for (int j = 0 ; j < n+1 ; j++) {
            dp[i][j] = 0;
        }
    }

	// intialising the table
    /*
	for (i = 0; i <= (n+m); i++)
	{
		dp[i][0] = i * pgap;
		dp[0][i] = i * pgap;
	}
    */
	for (i = 0; i <= m; i++) {
		dp[i][0] = i * pgap;
    }
	for (i = 0; i <= n; i++) {
		dp[0][i] = i * pgap;
	}

	// calcuting the minimum penalty
	for (i = 1; i <= m; i++)
	{
		for (j = 1; j <= n; j++)
		{
			if (x[i - 1] == y[j - 1])
			{
				dp[i][j] = dp[i - 1][j - 1];
			}
			else
			{
				dp[i][j] = std::min(std::min(dp[i - 1][j - 1] + pxy ,
								   dp[i - 1][j] + pgap) ,
								dp[i][j - 1] + pgap );
			}
		}
	}

	// Reconstructing the solution
	int l = n + m; // maximum possible length

	i = m; j = n;

	int xpos = l;
	int ypos = l;

	// Final answers for the respective strings
	int xans[l+1], yans[l+1];

	while ( !(i == 0 || j == 0))
	{
		if (x[i - 1] == y[j - 1])
		{
			xans[xpos--] = (int)x[i - 1];
			yans[ypos--] = (int)y[j - 1];
			i--; j--;
		}
		else if (dp[i - 1][j - 1] + pxy == dp[i][j])
		{
			xans[xpos--] = (int)x[i - 1];
			yans[ypos--] = (int)y[j - 1];
			i--; j--;
		}
		else if (dp[i - 1][j] + pgap == dp[i][j])
		{
			xans[xpos--] = (int)x[i - 1];
			yans[ypos--] = (int)'_';
			i--;
		}
		else if (dp[i][j - 1] + pgap == dp[i][j])
		{
			xans[xpos--] = (int)'_';
			yans[ypos--] = (int)y[j - 1];
			j--;
		}
	}
	while (xpos > 0)
	{
		if (i > 0) xans[xpos--] = (int)x[--i];
		else xans[xpos--] = (int)'_';
	}
	while (ypos > 0)
	{
		if (j > 0) yans[ypos--] = (int)y[--j];
		else yans[ypos--] = (int)'_';
	}

	// Since we have assumed the answer to be n+m long,
	// we need to remove the extra gaps in the starting
	// id represents the index from which the arrays
	// xans, yans are useful
	int id = 1;
	for (i = l; i >= 1; i--)
	{
		if ((char)yans[i] == '_' && (char)xans[i] == '_')
		{
			id = i + 1;
			break;
		}
	}

	// Printing the final answer
    if (report) {
	    std::cout << "Minimum Penalty in aligning the genes = ";
	    std::cout << dp[m][n] << "\n";
	    std::cout << "The aligned genes are :\n";
	    for (i = id; i <= l; i++)
	    {
		    std::cout<<(char)xans[i];
	    }
	    std::cout << "\n";
	    for (i = id; i <= l; i++)
	    {
		    std::cout << (char)yans[i];
	    }
    }
    int penalty = dp[m][n];
    for (i = 0 ; i < m+1 ; i++) {
        delete [] dp[i];
    }
    delete [] dp;
    return penalty;
}

#if 0
// Driver code
int main(){
	// input strings
	std::string gene1 = "adios2::Engine::BeginStep(const adios2::StepMode, const float)";
	std::string gene2 = "adios2::Engine::BeginStep(adios2::StepMode, float)";

	// intialsing penalties of different types
	int misMatchPenalty = 3;
	int gapPenalty = 1;

	// calling the function to calculate the result
	getMinimumPenalty(gene1, gene2,
		misMatchPenalty, gapPenalty);
	return 0;
}
#endif

