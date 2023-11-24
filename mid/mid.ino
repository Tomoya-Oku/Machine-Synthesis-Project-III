/*0: Sチキ，1: Lチキ*/
int MODE = 1;

/*Sチキ→0: 直進，1: 成功*/
/*Lチキ→0: 直進，1: カーブ前半，2: カーブ後半，3: 最後の直進．4: 成功*/
int PHASE = 0;

const int TEMPERATURE = 20;
const double SPEED_OF_SOUND = 331.5 + 0.6 * TEMPERATURE;

/*距離関連パラメータ*/
const int S_DISTANCE = 70;	// Sチキでの壁との目標距離[mm]
const int L_DISTANCE = 450; // Lチキでの曲がり始め時の壁までの距離[mm]
const int WALL_TIMES = 5;	// 壁検知の基準回数（この回数だけ基準値を下回ったら停止）

/*黒線関連パラメータ*/
const int BLACK_VALUE = 500; // 黒線検知の輝度基準値 //TODO : 値の変更
const int BLACK_TIMES = 1;	 // 黒線検知の基準回数（この回数だけ基準値を下回ったら停止）

/*超音波センサ（前）*/
const int ECHO_F = 2;
const int TRIG_F = 3;

/*超音波センサ（側面）*/
const int ECHO_S = 0;
const int TRIG_S = 1;

/*LED*/
const int LED1 = 4;

/*モータドライバ*/
/*A: 左タイヤ，B: 右タイヤ*/
const int AIN1 = 7;
const int AIN2 = 8;
const int BIN1 = 11;
const int BIN2 = 13;
const int PWMA = 9;
const int PWMB = 10;

/*DIPスイッチ*/
const int DIPS = 12;

/*フォトリフレクタ*/
const int PHOTO = A0;

/*ステッピングモータ*/

/*グローバル変数*/
int curve_flag = 0;	 // カーブの前半後半を示すフラグ
int L_speed = 255;	 // カーブの際の左タイヤスピード
int R_speed = 255;	 // カーブの際の右タイヤスピード
int black_count = 0; // 黒線検知回数
int wall_count = 0;	 // 距離検知回数
// double start_angle = 0; // カーブ開始時に取得する初期角度

void setup()
{
	Serial.begin(57600);

	pinMode(ECHO_F, INPUT);
	pinMode(TRIG_F, OUTPUT);
	pinMode(ECHO_S, INPUT);
	pinMode(TRIG_S, OUTPUT);

	pinMode(AIN1, OUTPUT);
	pinMode(AIN2, OUTPUT);
	pinMode(BIN1, OUTPUT);
	pinMode(BIN2, OUTPUT);
	pinMode(PWMA, OUTPUT);
	pinMode(PWMB, OUTPUT);

	pinMode(LED1, OUTPUT);

	pinMode(DIPS, INPUT);

	//MODE = digitalRead(DIPS); // DIPスイッチからモード取得

	digitalWrite(LED1, LOW); // LEDをオフで初期化

	Report("Setup has finished.");
}

void loop()
{
	/*直進チキンレース*/
	if (MODE == 0)
	{
		switch (PHASE)
		{
		case 0:
		{
			/*壁に近づくまで直進*/
			if (getDistance(TRIG_F, ECHO_F) > S_DISTANCE)
			{
				goStraight();
			}
			else
			{
				wall_count++;

				/*指定回数だけ壁を検知したら停止*/
				if (wall_count > WALL_TIMES)
				{
					PHASE = 1;		// 停止フェーズへ
					setSpeed(0, 0); // ブレーキ
					Report("Phase 0 has finished.");
				}
			}
			break;
		}
		case 1:
		{
			success(); // 成功後の処理を繰り返す
			Report("Mission completed.");
			break;
		}
		}
	}
	/*Lチキンレース*/
	else
	{
		switch (PHASE)
		{
		/*直進フェーズ*/
		case 0:
		{
			/*曲がり角まで直進*/
			if (getDistance(TRIG_F, ECHO_F) > L_DISTANCE)
			{
				goStraight();
			}
			else
			{
				// start_angle = getAngle(); // 初期角度を取得
				wall_count++;

				/*指定回数だけ壁を検知したら停止*/
				if (wall_count > WALL_TIMES)
				{
					PHASE = 1; // 停止フェーズへ
					Report("Phase 0 has finished.");
				}
			}
			break;
		}

		/*カーブフェーズ*/
		case 1:
		{
			if (hasCurved())
			{
				PHASE = 2; // 最後の直進フェーズへ
				Report("Phase 1 has finished.");
			}
			else
			{
				Curve();
        delay(5);
			}
			break;
		}

		/*最後の直進*/
		case 2:
		{
			if (getPhotoValue() >= BLACK_VALUE) // 黒線検知
			{
				black_count++;

				/*基準回数以上黒線を検知したら停止*/
				if (black_count >= BLACK_TIMES)
				{
          //delay(200);
					setSpeed(0, 0);
					PHASE = 3; // 成功フェーズへ
					Report("Phase 2 has finished.");
				}
			}
			else // 黒線検知まで直進
			{
				goStraight();
			}
			break;
		}

		/*成功*/
		case 3:
		{
			success(); // 成功後の処理を繰り返す
      Report("Phase 3 has finished.");
			break;
		}
		}
	}
}

/*成功後の処理*/
void success()
{
	digitalWrite(LED1, HIGH);
	delay(500);
	digitalWrite(LED1, LOW);
	delay(500);
}

/*両タイヤを与えられたスピード（-255~255）に設定*/
void setSpeed(int L_speed, int R_speed)
{
	if (L_speed >= 0)
	{
		digitalWrite(AIN1, HIGH);
		digitalWrite(AIN2, LOW);
		analogWrite(PWMA, L_speed);
	}
	else
	{
		digitalWrite(AIN1, LOW);
		digitalWrite(AIN2, HIGH);
		analogWrite(PWMA, -L_speed);
	}

	if (R_speed >= 0)
	{
		digitalWrite(BIN1, HIGH);
		digitalWrite(BIN2, LOW);
		analogWrite(PWMB, R_speed);
	}
	else
	{
		digitalWrite(BIN1, LOW);
		digitalWrite(BIN2, HIGH);
		analogWrite(PWMB, -R_speed);
	}
}

/*直進処理*/
void goStraight()
{
	setSpeed(235, 255);
	// TODO: 直進性
}

/*カーブの処理*/
// TODO: 要編集
void Curve()
{
	/*カーブ前半→右タイヤを減速*/
	if (curve_flag == 0)
	{
		R_speed--;
	}
	/*カーブ後半→右タイヤを加速*/
	else
	{
		R_speed++;
	}

	setSpeed(L_speed, R_speed);
}

/*カーブが終了したことを検知*/
// TODO: 要編集
bool hasCurved()
{
	if (R_speed == 75)
	{
		return true;
	}
	else
	{
		return false;
	}
}

/*フォトリフレクタの輝度を取得*/
int getPhotoValue()
{
	return analogRead(PHOTO);
}

/*現在の角度を取得*/
/*
double getAngle()
{
	//TODO : 処理内容
}
*/

/*超音波センサにより壁までの距離[mm]を計算*/
double getDistance(int trig_pin, int echo_pin)
{
	digitalWrite(trig_pin, LOW);
	delayMicroseconds(2);
	digitalWrite(trig_pin, HIGH);
	delayMicroseconds(10);
	digitalWrite(trig_pin, LOW);
	double duration = pulseIn(echo_pin, HIGH); // 往復にかかった時間が返却される[ms]

	if (duration > 0)
	{
		duration = duration / 2; // 往路にかかった時間[ms]

		// int temperature = getTemp();
		double distance = duration * SPEED_OF_SOUND * 100 / 100000;

		return distance;
	}
}

/*気温[℃]を取得*/
/*
int getTemp()
{
	int temp;
	int val = analogRead(0);
	val = map(val, 0, 1023, 0, 3300);
	temp = map(val, 100, 1750, -40, 125);

	return temp;
}
*/

/*シリアルモニタにて状況報告*/
void Report(String message)
{
	Serial.println("Report");
	Serial.print("[Message] ");
	Serial.println(message);

	Serial.print("[Mode] ");
	Serial.print(MODE);
	Serial.print("\t[Phase] ");
	Serial.print(PHASE);

	Serial.print("\n[Distance] ");
	Serial.print(getDistance(TRIG_F, ECHO_F));
	Serial.print("[mm]");

	Serial.print("\t[Brightness] ");
	Serial.print(getPhotoValue());

	/*
	Serial.print("\tDistance (side): ");
	Serial.print(getDistance(TRIG2, ECHO2));
	Serial.print("[mm]");
	*/

	/*
	Serial.print("\t[Temperature] ");
	Serial.print(getTemp());
	Serial.print("[℃]");
	*/

	Serial.println("\n[Global variables]");
	Serial.print("int R_speed: ");
	Serial.print(R_speed);
	Serial.print("\tint black_count: ");
	Serial.print(black_count);
	Serial.print("\tint wall_count: ");
	Serial.print(wall_count);
	Serial.print("\n");
}