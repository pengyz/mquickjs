
// Complex module example
function fibonacci(n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

function factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

var MathModule = {
    fib: fibonacci,
    fact: factorial,
    name: 'Math Module',
    version: '1.0'
};

