#include<cstdio>
#include<iostream>
#include<sstream>
#include<cstdlib>
#include<cmath>
#include<cctype>
#include<string>
#include<cstring>
#include<algorithm>
#include<stack>
#include<queue>
#include<set>
#include<map>
#include<ctime>
#include<vector>
#include<fstream>
#include<list>
#include<iomanip>

using namespace std;

const int counts = 3; //系数的个数-（x+1）

//解线性方程组-高斯列主元素消元法
void swap_row(int row1, int row2, double matrix[][counts+1]) {  //交换两行元素
    double *d = (double*)malloc((counts + 1) * sizeof(double));

    memcpy(d, matrix[row1], (counts + 1) * sizeof(double));
    memcpy(matrix[row1], matrix[row2], (counts + 1) * sizeof(double));
    memcpy(matrix[row2], d, (counts + 1) * sizeof(double));
}

void find_max(int k, double matrix[][counts+1]) {//列元素中找最大值
    int r = k;
    double maxs = abs(matrix[k][k]);
    for(int i = k; i < counts; i++) {
        if(abs(matrix[i][k]) > maxs) {
            r = i;
            maxs = abs(matrix[i][k]);
        }
    }
    swap_row(k, r, matrix);
}

void LinearEquations(double matrix[][counts+1], double Answer[]) {
    //消元过程
    for(int i = 0; i < counts-1; i++) {
        find_max(i, matrix);//使主元素选取相较更大的值

        for(int j = i+1; j < counts; j++) {//第j行
            double l = matrix[j][i] / matrix[i][i];
            for(int k = i; k <= counts; k++)//j行的每一个元素
                matrix[j][k] = matrix[j][k] - l*matrix[i][k];
        }
    }

    //回代过程
    for(int i = counts-1; i >= 0; i--) {
        double sum = 0;
        for(int j = i+1; j < counts; j++)//求累加和
            sum += matrix[i][j] * Answer[j];
        Answer[i] = (matrix[i][counts] - sum) / matrix[i][i];
    }
}

void MultipleRegression(double data[][counts], int rows, int cols, double Answer[], double SquarePoor[]) {
    int count = cols - 1;

    double a, b;

    //打表回归系数方程组
    double dat[counts][counts+1];
    dat[0][0] = rows;
    for (int n = 0; n < count; n++) {                     // n = 0 to cols - 2,不包括cols-1这一列的Y
        a = b = 0.0;
        for (int m = 0; m < rows; m++) {//遍历每一行的第n列
            a += data[m][n];
            b += (data[m][n] * data[m][n]);
        }
        dat[0][n+1] = a;                              // dat[0, n+1] = Sum(Xn)
        dat[n+1][0] = a;                               // dat[n+1, 0] = Sum(Xn)
        dat[n+1][n+1] = b;                            // dat[n+1,n+1] = Sum(Xn * Xn)
        for (int i = n + 1; i < count; i ++) {            // i = n+1 to cols - 2
            a = 0;
            for (int m = 0; m < rows; m++)
                a += data[m][n] * data[m][i];
            dat[n+1][i+1] = a;        // dat[n+1, i+1] = Sum(Xn * Xi)
            dat[i+1][n+1] = a;   // dat[i+1, n+1] = Sum(Xn * Xi)
        }
    }
    b = 0;
    for (int m = 0; m < rows; m++)
        b += data[m][count];
    dat[0][cols] = b;                                   // dat[0, cols] = Sum(Y)
    for (int n = 0; n < count; n++) {
        a = 0;
        for (int m = 0; m < rows; m++)
            a += data[m][n] * data[m][count];
        dat[n+1][cols] = a;        // dat[n+1, cols] = Sum(Xn * Y)
    }

    LinearEquations(dat, Answer);          // 计算方程式
    // 方差分析
    if (SquarePoor) {
        b = b / rows;                                // b = Y的平均值
        SquarePoor[0] = SquarePoor[1] = 0.0;
        for (int m = 0; m < rows; m++) {
            a = Answer[0];
            for (int i = 1; i < cols; i++)
                a += (data[m][i-1] * Answer[i]);               // a = Ym的估计值
            SquarePoor[0] += ((a - b) * (a - b));    // U(回归平方和)
            SquarePoor[1] += ((data[m][count] - a) * (data[m][count] - a));  // Q(剩余平方和)(*p = Ym)
        }
        SquarePoor[2] = SquarePoor[0] / count;       // 回归方差
        if(rows - cols > 0.0)
            SquarePoor[3] = SquarePoor[1] / (rows - cols); // 剩余方差
        else
            SquarePoor[3] = 0.0;
    }
}

double calculate(double x[counts-1], double Answer[]) {
    double ans = Answer[0];
    for(int i = 1; i < counts; i++)
        ans += Answer[i] * x[i-1];
    return ans;
}

/*
int main() {
    double data[50][counts];

    for(int i = 0; i < 40; i++) {
        for(int j = 0; j < counts; j++) {
            cin >> data[i][j];
            getchar();
        }
    }

    double Answer[counts], SquarePoor[4];
    MultipleRegression(data, 40, counts, Answer, SquarePoor);

    double x[10];
    for(int i = 0; i < 7; i++) {
        for(int j = 0; j < counts-1; j++) {
            cin >> x[j];
            getchar();
        }

        cout << calculate(x, Answer) << endl;;
    }

    return 0;
}
*/
