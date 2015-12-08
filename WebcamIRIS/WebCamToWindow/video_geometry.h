#pragma once
#include "video_analysis.h"

template<typename T, typename number>
int geom_linear_least_squares(T& point_list, number* m, number* b, number* r) // y=mx+b
{
	number   sumx = 0.0;                        /* sum of x                      */
	number   sumx2 = 0.0;                       /* sum of x**2                   */
	number   sumxy = 0.0;                       /* sum of x * y                  */
	number   sumy = 0.0;                        /* sum of y                      */
	number   sumy2 = 0.0;                       /* sum of y**2                   */

	for (T::iterator it = point_list.begin();it != point_list.end();it++)
	{
		sumx += it->ix;
		sumx2 += pow2(it->ix);
		sumxy += it->ix * it->iy;
		sumy += it->iy;
		sumy2 += pow2(it->iy);
	}

	number    denom = (point_list.size() * sumx2 - pow2(sumx));
	if (denom == 0) {
		// singular matrix. can't solve the problem.
		*m = 0;
		*b = 0;
		*r = 0;
		return 1;
	}

	*m = (point_list.size() * sumxy - sumx * sumy) / denom;
	*b = (sumy * sumx2 - sumx * sumxy) / denom;
	if (r != NULL) {
		*r = (sumxy - sumx * sumy / point_list.size()) /          /* compute correlation coeff     */
			sqrt((sumx2 - pow2(sumx) / point_list.size()) *
				(sumy2 - pow2(sumy) / point_list.size()));
	}

	return 0;
}

template<typename T>
void geom_aligned_points_search(T point_list, int iter_max=100)
{
	for(int niter=0;niter<iter_max;niter++)
	{

	for (T::iterator it = point_list.begin();it != point_list.end();it++)
		for (T::iterator ic = point_list.begin();ic != point_list.end();ic++)
		{

		}
	}
}