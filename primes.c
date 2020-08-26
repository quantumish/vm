// Very naive primality test

int main()
{
    int n = 31;
    int prime = 1;
    for (int i = 2; i*i < n; i++) {
        if (n % i == 0) prime = 0;
    }
}
