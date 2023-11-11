/*モード切り替え*/
int MODE = 0; //0: 直進チキンレース，1: Lチキ

const int S_DISTANCE = 5; //直進チキンレースでの壁との目標距離[mm]
const int L_DISTANCE = 200; //Lチキンレースでの曲がり始め時の壁までの距離[mm]

const int BLACK_VALUE = 200; //黒線検知の基準値

const double SPEED_OF_SOUND = 331.5 + 0.6 * 25; //音速，25℃の気温の想定

/*超音波センサ*/
const int ECHO = 2;
const int TRIG = 3;

/*LED*/
const int LED1 = 4;

/*モータドライバ*/
const int AIN1 = 7;
const int AIN2 = 8;
const int PWMA = 9; //左タイヤ
const int PWMB = 10; //右タイヤ

/*DIPスイッチ*/
const int DIPS = 12;

/*フォトリフレクタ*/
const int PIN1 = A0;

/*ステッピングモータ*/

/*フェーズ*/
int S_phase = 0; //直進チキンレース用，0: 直進，1: ブレーキ，2: 成功
int L_phase = 0; //Lチキ用，0: 初めの直進，1: カーブ中，2: 最後の直進．3: 成功

double start_angle = 0; //カーブ開始時に取得する初期角度

void setup()
{
	Serial.begin(57600);

	pinMode(ECHO, INPUT);
	pinMode(TRIG, OUTPUT);

	pinMode(AIN1, OUTPUT);
	pinMode(AIN2, OUTPUT);
	pinMode(PWMA, OUTPUT);
	pinMode(PWMB, OUTPUT);

	pinMode(LED1, OUTPUT);

	pinMode(DIPS, INPUT);

	MODE = digitalRead(DIPS); //DIPスイッチからモード取得
}

void loop()
{
	if (MODE == 0) //直進モード
	{
		switch (S_phase)
		{
			case 0:
			 	/*壁に近づくまで直進*/
				if (getDistance() >= S_DISTANCE)
				{
					setSpeed(255, 255); //maxスピードで直進
				}
				else
				{
					S_phase = 1; //停止フェーズへ
				}
				break;
			case 1:
				setSpeed(0, 0); //ブレーキ
				S_phase = 2; //成功フェーズへ
				break;
			case 2:
				success(); //成功後の処理を繰り返す
				break;
		}
	}
	else //Lチキ
	{
		switch (phase)
		{
			/*最初の直進*/
			case 0:
				/*曲がり角まで直進*/
				if (getDistance() <= L_DISTANCE)
				{
					setSpeed(255, 255);
				}
				else
				{
					start_angle = getAngle(); //初期角度を取得
					phase = 1; //カーブに到達→カーブフェーズへ
				}
				break;

			/*カーブ中*/
			case 1:
				//初期角度 + 90[deg]になればカーブ終了
				if (getAngle() >= start_angle + 90) #TODO: ラジアンか確認
				{
					phase = 2; //最後の直進フェーズへ
				}
				else
				{
					curve();
				}
				break;

			/*最後の直進*/
			case 2:
				if (isOnLine()) //黒線検知
				{
					setSpeed(0, 0);
					phase = 3; //成功フェーズへ
				}
				else //黒線検知まで直進
				{
					setSpeed(255, 255);
				}
				break;

			/*成功*/
			case 3:
				success(); //成功後の処理を繰り返す
				break;
		}
	}
}

/*成功後の処理*/
void success()
{
	digitalWrite(2, HIGH);
	delay(1000);
	digitalWrite(2, LOW);
	delay(1000);
}

/*両タイヤを与えられたスピード（0〜255）に*/
void setSpeed(int L_speed, int R_speed)
{
	digitalWrite(AIN1, HIGH);
	digitalWrite(AIN2, LOW);
	analogWrite(PWMA, L_speed);
	analogWrite(PWMB, R_speed);
}

/*カーブを曲がる*/
void curve() 
{
	setSpeed(255, 50); # TODO: 処理の追加
}

/*黒線検知*/
bool isOnLine() #TODO: ノイズへの対応
{
	int value = analogRead(PIN1);
	Serial.println(value);

	if (value <= BLACK_VALUE) #TODO: 値の変更
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*現在の角度を取得*/
double getAngle()
{
	#TODO: 処理内容
}

/*超音波センサにより壁までの距離を計算*/
/*単位はmm*/
double getDistance()
{
    digitalWrite(TRIG, LOW); 
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10); 
    digitalWrite(TRIG, LOW);
    double duration = pulseIn(ECHO, HIGH); //往復にかかった時間が返却される[ms]

    if (duration > 0)
    {
		duration = duration / 2; //往路にかかった時間
		double distance = duration * speed_of_sound * 100 / 100000;

		/*
		Serial.print("Distance:");
		Serial.print(distance);
		Serial.println(" cm");
		*/

      	return distance;
    }
}
