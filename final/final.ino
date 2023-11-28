// TODO: 機体角度を取得できるようにしたい

/*動作モード*/
enum Mode
{
	AUTO,	// 自動制御
	MANUAL, // 遠隔操縦
	TEST	// テスト
};

/*自動制御におけるフェーズ*/
enum Phase
{
	PUSHING,	 // カウンターに2つのドリンクを押し込むフェーズ
	FINDING,	 // テーブルに置くドリンクを発見するフェーズ
	APPROACHING, // 発見したドリンクに近づくフェーズ
	LIFTING,	 // ドリンクを持ち上げるフェーズ
	SEARCHING,	 // テーブルを探すフェーズ
	CARRYING,	 // テーブルまでドリンクを運ぶフェーズ
	CHECKING,	 // テーブルの温度とドリンクの色が一致しているか調べるフェーズ
	PUTTING,	 // テーブルにドリンクを置くフェーズ
	SUCCESS		 // すべてのミッション成功後に繰り返すフェーズ
};

/*初期値*/
Mode mode = TEST;
Phase phase = PUSHING;

/*距離関連パラメータ*/
const int DISTANCE_COUNT = 1;  // 距離検知の基準回数（この回数だけ基準値を下回ったら停止）
int AIR_TEMPERATURE = 20;	   // 初期化により変更される
double SPEED_OF_SOUND = 343.5; // 気温20℃での値

/*黒線関連パラメータ*/
const int BLACK_VALUE = 500; // 黒線検知の輝度基準値
const int BLACK_COUNT = 1;	 // 黒線検知の基準回数（この回数だけ基準値を下回ったら停止）

/*温度関連パラメータ*/
const int HOT_VALUE = 40; // テーブルがHOTと判断する基準値[℃]

/*アーム関連パラメータ*/
const int CARRYING_DISTANCE = 50; // ドリンクを運ぶ際に近づく距離[mm]

/**********ピン番号***********/
/*超音波センサ*/
const int ECHO = 2;
const int TRIG = 3;
/*モータドライバ(A: 左タイヤ，B: 右タイヤ)*/
const int AIN1 = 7;
const int AIN2 = 8;
const int BIN1 = 11;
const int BIN2 = 13;
const int PWMA = 9;
const int PWMB = 10;
/*フォトリフレクタ*/
const int PHRB = A0; // 黒線検知用
const int PHRD = A1; // ドリンク色検知用

/**********グローバル変数***********/
int distance_counter = 0; // 距離検知回数
int black_counter = 0;	  // 黒線検知回数
int achievement_flag = 0; // テーブル達成状況
bool arm_is_open = true;  // アームが開いているかどうか
bool arm_is_down = true;  // アームが下がっているかどうか

void setup()
{
	Serial.begin(57600);

	/*超音波センサ*/
	pinMode(ECHO, INPUT);
	pinMode(TRIG, OUTPUT);

	/*モータドライバ*/
	pinMode(AIN1, OUTPUT);
	pinMode(AIN2, OUTPUT);
	pinMode(BIN1, OUTPUT);
	pinMode(BIN2, OUTPUT);
	pinMode(PWMA, OUTPUT);
	pinMode(PWMB, OUTPUT);

	/*気温を取得し，音速を計算*/
	AIR_TEMPERATURE = getTemp();
	SPEED_OF_SOUND = 331.5 + 0.6 * AIR_TEMPERATURE;

	/*アームを開く*/
	armOpen();

	Report("Setup has finished.");
}

void loop()
{
	/*シリアルモニタでコマンド受付*/
	if (Serial.available())
	{
		String command = Serial.readStringUntil('\n'); // シリアルから文字列を受信
		command.toLowerCase();						   // 入力された文字列を小文字に
		Response(command);
	}

	/*自律制御*/
	if (mode == AUTO)
	{
		switch (phase)
		{
		/*最初に2個のドリンクを押すフェーズ*/
		case PUSHING:
		{
			/*カウンターまでドリンクを入れたとき*/
			if (isInCounter())
			{
				Report("I put two drinks on the counter.");
				phase = FINDING;
			}
			else
			{
				// armOpen();
				goStraight(255); // 直進
			}
			break;
		}

		/*他のドリンクを探すフェーズ*/
		case FINDING:
		{
			findDrink(); // ドリンクを探す

			Report("I found another drink.");
			phase = APPROACHING;
			break;
		}

		/*検出したドリンクまで向かうフェーズ*/
		case APPROACHING:
		{
			/*適当な距離までドリンクに近づく*/
			if (getDistance(TRIG, ECHO) <= CARRYING_DISTANCE)
			{
				distance_counter++; // ノイズ対策
				if (distance_counter >= DISTANCE_COUNT)
				{
					Report("I approached the drink.");
					distance_counter = 0; // リセット
					phase = LIFTING;
				}
			}
			else
			{
				goStraight(255); // 直進
			}
			break;
		}

		/*ドリンクを持ち上げるフェーズ*/
		case LIFTING:
		{
			armClose(); // アームを閉じる
			delay(1000);
			armUp(); // アームを上げる
			phase = SEARCHING;
			break;
		}

		/*ドリンクを置くテーブルを探すフェーズ*/
		case SEARCHING:
		{
			findTable(); // テーブルを探す

			Report("I found a table.");
			phase = CARRYING;
			break;
		}

		/*テーブルまでドリンクを運ぶフェーズ*/
		case CARRYING:
		{
			/*適当な距離までテーブルに近づく*/
			if (getDistance(TRIG, ECHO) <= CARRYING_DISTANCE)
			{
				distance_counter++; // ノイズ対策
				if (distance_counter >= DISTANCE_COUNT)
				{
					Report("I carried the drink.");
					distance_counter = 0; // リセット
					phase = CHECKING;
				}
			}
			else
			{
				goStraight(255); // 直進
			}
			break;
		}

		/*テーブルの温度を調べるフェーズ*/
		case CHECKING:
		{
			/*テーブルの温度とドリンクの色が一致*/
			int temp = getTemp();
			if ((temp >= HOT_VALUE && isBlack()) || (temp < HOT_VALUE && !isBlack()))
			{
				Report("The temperature of the table corresponds with the color of the drink.");
				phase = PUTTING;
			}
			/*テーブルの温度とドリンクの色が一致しない*/
			else
			{
				Report("The temperature of the table DOESN'T correspond with the color of the drink.");
				phase = SEARCHING;
			}
			break;
		}

		/*テーブルにドリンクを置くフェーズ*/
		case PUTTING:
		{
			armDown(); // アームを下ろす
			delay(1000);
			armOpen(); // アームを開く
			delay(1000);
			goStraight(-255); // 後退する
			delay(1000);

			Report("I put the drink on the table.");

			achievement_flag++; // 達成フラグを1増やす
			/*2つのドリンクを正しくテーブルに乗せた*/
			if (achievement_flag >= 2)
			{
				Report("I have accomplished all the missions.");
				phase = SUCCESS;
			}
			else
			{
				Report("I'll find the other drink.");
				phase = FINDING;
			}
			break;
		}

		/*成功後のフェーズ*/
		case SUCCESS:
			Success();
			break;
		}
	}
	/*遠隔操縦*/
	else if (mode == MANUAL)
	{
	}
	/*テスト*/
	else if (mode == TEST)
	{
	}
}

/*アームを開く*/
void armOpen()
{
	/*アームが開いていないとき*/
	if (!arm_is_open)
	{
		// TODO: 要編集
	}
}

/*アームを閉じる*/
void armClose()
{
	/*アームが開いているとき*/
	if (arm_is_open)
	{
		// TODO: 要編集
	}
}

/*アームを上げる*/
void armUp()
{
	/*アームが下がっているとき*/
	if (arm_is_down)
	{
		// TODO: 要編集
	}
}

/*アームを下げる*/
void armDown()
{
	/*アームが上がっているとき*/
	if (!arm_is_down)
	{
		// TODO: 要編集
	}
}

/*カウンター内に入ったかどうか*/
bool isInCounter()
{
	// TODO: 要編集
}

/*ドリンクを探す→探し終えたら終了*/
void findDrink()
{
	// TODO: 要議論（どうやって探すか，既に運び終わったドリンクをどうやって見分けるか）・要編集
}

/*テーブルを探す→探し終えたら終了*/
void findTable()
{
	// TODO: 要議論（どうやって探すか，既にドリンクが置いてあるテーブルをどうやって見分けるか，目の前にあるテーブルともう一つのテーブルをどうやって見分けるか）・要編集
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

/*与えられたスピード（-255~255）で直進*/
void goStraight(int speed)
{
	setSpeed(speed, speed);
}

/*与えられた角度（-180[deg]~180[deg]に回転*/
void rotate(int angle)
{
	// TODO: 要編集
}

/*ドリンクの色を判定*/
bool isBlack()
{
	// TODO: 要議論（いつ判定するか）・要編集
}

/*フォトリフレクタの輝度を取得*/
int getBrightness()
{
	return analogRead(PHRB);
}

/*超音波センサで壁までの距離[mm]を計算*/
double getDistance(int trig, int echo)
{
	digitalWrite(trig, LOW);
	delayMicroseconds(2);
	digitalWrite(trig, HIGH);
	delayMicroseconds(10);
	digitalWrite(trig, LOW);
	double duration = pulseIn(echo, HIGH); // 往復にかかった時間が返却される[ms]

	if (duration > 0)
	{
		duration = duration / 2; // 往路にかかった時間[ms]
		double distance = duration * SPEED_OF_SOUND * 100 / 100000;
		return distance;
	}
}

/*温度[℃]を取得*/
int getTemp()
{
	// TODO: 要編集
}

/*成功後の動作*/
void Success()
{
	// TODO: 要議論・要編集
}

/*シリアルモニタにて状況報告*/
void Report(String message)
{
	Serial.print("\n[Report] ");
	Serial.print(message);
}

/*シリアルモニタでコマンドに対して応答*/
void Response(String command)
{
	if (command == "phase" || command == "ph")
	{
		Serial.print("\n[Phase] ");
		Serial.print(phase);
	}
	else if (command == "mode")
	{
		Serial.print("\n[Mode] ");
		Serial.print(mode);
	}
	else if (command == "mode_change auto" || command == "mc auto")
	{
		mode = AUTO;
		phase = PUSHING;
	}
	else if (command == "mode_change manual" || command == "mc manual" || command == "mc man")
	{
		mode = MANUAL;
	}
	else if (command == "mode_change test" || command == "mc test")
	{
		mode = TEST;
	}
	else if (command == "distance" || command == "dist")
	{
		Serial.print("\n[Distance] ");
		Serial.print(getDistance(TRIG, ECHO));
		Serial.print("[mm]");
	}
	else if (command == "brightness" || command == "brt")
	{
		Serial.print("\n[Brightness] ");
		Serial.print(getBrightness());
	}
	else if (command == "temperature" || command == "temp")
	{
		Serial.print("\n[Temperature] ");
		Serial.print(getTemp());
		Serial.print("[℃]");
	}
	else if (command == "distance_counter" || command == "dc")
	{
		Serial.print("\n[Distance Counter] ");
		Serial.print(distance_counter);
	}
	else if (command == "black_line_counter" || command == "blc")
	{
		Serial.print("\n[Black Line Counter] ");
		Serial.print(black_counter);
	}
	else if (command == "air_temperature" || command == "atemp")
	{
		Serial.print("\n[Air temperature] ");
		Serial.print(AIR_TEMPERATURE);
		Serial.print("[℃]");
	}
	else if (command == "speed of sound" || command == "ss")
	{
		Serial.print("\n[Speed of sound] ");
		Serial.print(SPEED_OF_SOUND);
		Serial.print("[m/s]]");
	}
	else
	{
		Serial.print("\nI don't know the meaning of \"");
		Serial.print(command);
		Serial.print("\"");
	}
}