# alphaFlexWalk
## compile
`g++ -o get_paths.out get_paths.cpp Graph.cpp Index.cpp`
## run
`./get_paths.out [dataset name (like test)]` 
## output example
```
Index for alpha_index = 0.4
0
0 1 
0 1 
0 1 
0 1 0 
0 2 4 

1
1 0 1 
1 0 1 0 
1 0 1 
1 2 5 
1 0 
1 0 
1 3 6 7 
1 3 

2
2 4 3 
2 5 4 
2 5 
2 5 
2 4 6 9 

3
3 6 9 
3 6 
3 6 9 5 

4
4 6 7 
4 1 2 5 
4 1 2 5 4 
4 6 
4 3 
4 3 6 
4 3 
4 3 6 9 

5
5 10 11 
5 10 
5 4 
5 4 
5 4 3 6 7 3 6 

6
6 9 
6 7 
6 7 3 
6 7 
6 7 

7
7 3 6 
7 8 
7 3 
7 8 9 
7 8 9 

8
8 9 4 3 
8 11 9 
8 11 
8 9 
8 9 4 3 

9
9 4 
9 5 
9 4 
9 4 6 7 
9 5 4 

10
10 11 
10 11 
10 11 5 

11
11 5 
11 5 4 
11 9 
11 9 
11 9 

Start Query
Paths for alpha = 0.1
0 1 0 1 2 
0 1 2 5 10 11 
0 1 0 1 2 5 10 11 5 4 1 2 5 4 6 7 3 6 7 8 9 4 
0 1 0 2 5 4 
0 2 4 6 7 3 6 9 
0 1 0 1 0 
0 2 4 3 6 9 
0 1 3 6 7 8 
0 1 3 6 7 3 6 9 5 4 1 2 5 
0 1 0 1 0 

Paths for alpha = 0.4
0 1 
0 1 
0 1 
0 1 0 
0 2 4 
0 2 4 3 
0 1 0 1 
0 1 0 1 0 
0 1 
0 

Paths for alpha = 0.7
0 1 
0 1 
0 1 
0 1 0 
0 2 4 
0 
0 
0 
0 
0 
```
