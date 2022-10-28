int led1 = 0; //Specify to which pin you are connecting the Left Orange LED
int led2 = 1; //Specify to which pin you are connecting the Middle Orange LED
int led3 = 2; //Specify to which pin you are connecting the Right Orange LED
int led4 = 4; //Specify to which pin you are connecting the Red LED
int topsw = 3; //Specify to which pin you are connecting the trigger switch
bool animationRunning = false;

void setup() {
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(topsw, INPUT_PULLUP);
}

void loop()
{
  if(animationRunning == false && digitalRead(topsw) == LOW)
  {
    animationRunning = true;
    runAnimation();
  }
}

void runAnimation() {

  //Light Status 1
  digitalWrite(led4, HIGH);
  digitalWrite(led1, HIGH);
  delay(600);
  digitalWrite(led1, LOW);
  delay(600);

  //Light Status 2
  digitalWrite(led2, HIGH);
  delay(600);
  digitalWrite(led2, LOW);
  delay(600);

  //Light Status 3
  digitalWrite(led3, HIGH);
  delay(600);
  digitalWrite(led3, LOW);
  delay(600);

  //Light Status 4
  digitalWrite(led1, HIGH);
  digitalWrite(led2, HIGH);
  delay(600);
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  delay(600);

  //Light Status 5
  digitalWrite(led1, HIGH);
  digitalWrite(led3, HIGH);
  delay(600);
  digitalWrite(led1, LOW);
  digitalWrite(led3, LOW);
  delay(600);

  //Light Status 6
  digitalWrite(led3, HIGH);
  delay(600);
  digitalWrite(led3, LOW);
  delay(600);

  //Light Status 7
  digitalWrite(led1, HIGH);
  delay(600);
  digitalWrite(led1, LOW);
  delay(600);

  //Light Status 8
  digitalWrite(led2, HIGH);
  digitalWrite(led3, HIGH);
  delay(600);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  delay(600);

  //Light Status 9
  digitalWrite(led1, HIGH);
  digitalWrite(led3, HIGH);
  delay(600);
  digitalWrite(led1, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);

  animationRunning = false;
}
