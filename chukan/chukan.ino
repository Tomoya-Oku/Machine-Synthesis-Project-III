/*超音波センサ*/
const int TRIG = 3;
const int ECHO = 2;

/*モータドライバ*/
const int AIN1 = 7;
const int AIN2 = 8;
const int PWMA = 9;
const int PWMB = 10;

double duration = 0;
double speed_of_sound = 331.5 + 0.6 * 25; // 25℃の気温の想定

void setup()
{
  Serial.begin(57600);

  pinMode(ECHO, INPUT);
  pinMode(TRIG, OUTPUT);

  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(PWMA, OUTPUT);
  pinMode(PWMB, OUTPUT);
}

void loop()
{
  double distance = calculateDistanceToWall();

	if (distance >= 5)
	{
    //加速
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, 255);
    //analogWrite(PWMB, 255);
	}
  else
  {
    //ブレーキ
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    analogWrite(PWMA, 0);
    //analogWrite(PWMB, 0);
  }

  /*
	if (黒線検出)
	{
		
	}
  */

}

/*超音波センサにより壁までの距離を計算*/
/*単位はcm*/
double calculateDistanceToWall()
{
    digitalWrite(TRIG, LOW); 
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10); 
    digitalWrite(TRIG, LOW);
    duration = pulseIn(ECHO, HIGH); //往復にかかった時間が返却される[ms]

    if (duration > 0)
    {
      duration = duration / 2; //往路にかかった時間
      double distance = duration * speed_of_sound * 100 / 1000000;
      Serial.print("Distance:");
      Serial.print(distance);
      Serial.println(" cm");

      return distance;
    }
}
