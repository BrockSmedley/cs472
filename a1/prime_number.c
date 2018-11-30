/*
 *         Short program to determine whether a provided positive number is prime. 
 *         */ 
int main() {  
  // do not change this section 
  // you MUST retain the following two variable names: 
  int num = 61;
  int is_prime; 
  // your code goes in between the following markers 
  // <------------->
  for (int i = num-1; i > 1; i--){
    if (num % i == 0){
      is_prime = 0;
      break;
    }
    else
      is_prime = 1;
  }
  // <------------->
  // should return 0 if "num" was not prime 
  // should return 1 if "num" was prime 
  return is_prime;
}
