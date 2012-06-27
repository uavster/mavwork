/*
 * ConsoleUI.cpp
 *
 *  Created on: 15/08/2011
 *      Author: Ignacio Mellado
 */

#include <ncurses.h>
#include <ConsoleUI.h>
#include <atlante.h>
#include <math.h>
#include <string.h>

#define LOGS_TO_FILE

#define PITCH_CMD	0.5

using namespace DroneProxy;
using namespace DroneProxy::Comm;

ConsoleUI::ConsoleUI(MyDrone *drone) {
	this->drone = drone;
	isInit = false;
	drone->addLogConsumer(this);
}

ConsoleUI::~ConsoleUI() {
	destroy();
	drone->removeLogConsumer(this);
}

void ConsoleUI::init() {
	if (consoleMutex.lock()) {
		if (!isInit) {
			isInit = true;

#ifdef LOGS_TO_FILE
			logFile = fopen("log.txt", "w");
			if (logFile == NULL)
				throw cvgException("[ConsoleUI] Cannot open log file");
#endif

			initscr();
			raw();
			keypad(stdscr, true);
			noecho();
			nodelay(stdscr, true);

			// Log window creation
			int row, col;
			getmaxyx(stdscr, row, col);
			logWin = newwin(row / 2, col, row / 2, 0);
			box(logWin, 0, 0);
			scrollok(logWin, true);
			wmove(logWin, 1, 0);
			refresh();
			wrefresh(logWin);

			memset(&lastDroneInfo, 0, sizeof(lastDroneInfo));

		}
		consoleMutex.unlock();
		drone->getChannelManager().getFeedbackChannel().addEventListener(this);
	} else throw cvgException("[ConsoleUI::init] Cannot lock console mutex");

	logConsume(NULL, "User interface ready");
}

void ConsoleUI::destroy() {
	if (consoleMutex.lock()) {
		if (isInit) {

			drone->getChannelManager().getFeedbackChannel().removeEventListener(this);

			// Destroy log window
			wborder(logWin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
			wrefresh(logWin);
			delwin(logWin);

			endwin();

			isInit = false;

			printf("[ConsoleUI] User interface closed\n");
#ifdef LOGS_TO_FILE
			fclose(logFile);
#endif
		}
		consoleMutex.unlock();
	}
}

void ConsoleUI::doLoop() {
/*	static cvg_double desiredYaw = 0.0;
	static cvg_double desiredAlt = 1.0;
*/
	FeedbackPacket p;

	bool loop = true;
	while(loop) {
		refreshInfo();
		switch(getch()) {
			case 27:
				loop = false;
				break;
			case KEY_UP:
				drone->setControlData(ControlChannel::THETA, 0.0f, -PITCH_CMD, 0.0f, 0.0f);
				break;
			case KEY_DOWN:
				drone->setControlData(ControlChannel::THETA, 0.0f, PITCH_CMD, 0.0f, 0.0f);
				break;
			case KEY_LEFT:
				drone->setControlData(ControlChannel::PHI, -1.0f, 0.0f, 0.0f, 0.0f);
				break;
			case KEY_RIGHT:
				drone->setControlData(ControlChannel::PHI, 1.0f, 0.0f, 0.0f, 0.0f);
				break;
			case '5':
				drone->setControlData(0.0f, 0.0f, 0.0f, 0.0f);
				break;
			case 'a':
				drone->setControlData(ControlChannel::GAZ, 0.0f, 0.0f, 0.0f, 0.0f);
				break;
			case 'q':
				drone->setControlData(ControlChannel::GAZ, 0.0f, 0.0f, 0.1f, 0.0f);
				break;
			case 's':
				drone->setControlData(ControlChannel::GAZ, 0.0f, 0.0f, -0.2f, 0.0f);
				break;
			case 'w':
				drone->setControlData(ControlChannel::GAZ, 0.0f, 0.0f, 0.2f, 0.0f);
				break;
			case 'd':
				drone->setControlData(ControlChannel::GAZ, 0.0f, 0.0f, -0.3f, 0.0f);
				break;
			case 'e':
				drone->setControlData(ControlChannel::GAZ, 0.0f, 0.0f, 0.3f, 0.0f);
				break;
			case 'f':
				drone->setControlData(ControlChannel::GAZ, 0.0f, 0.0f, -0.4f, 0.0f);
				break;
			case 'r':
				drone->setControlData(ControlChannel::GAZ, 0.0f, 0.0f, 0.4f, 0.0f);
				break;
			case 'h':
				drone->setControlMode(HOVER);
				break;
			case 'm':
				drone->setControlMode(MOVE);
				break;
			case ' ':
				drone->setControlMode(EMERGENCYSTOP);
				break;
			case 'l':
				drone->setControlMode(LAND);
				break;
			case 't':
				drone->setControlMode(TAKEOFF);
				break;
			case 'z':
				drone->setControlData(ControlChannel::YAW, 0.0f, 0.0f, 0.0f, -0.1f);
				break;
			case 'x':
				drone->setControlData(ControlChannel::YAW, 0.0f, 0.0f, 0.0f, 0.1f);
				break;
/*			case 'g':
				desiredYaw += 10.0 * M_PI / 180.0;
				drone->setYaw(desiredYaw);
				break;
			case 'f':
				desiredYaw -= 10.0 * M_PI / 180.0;
				drone->setYaw(desiredYaw);
				break;
			case 'i':
				desiredAlt += 0.25;
				drone->setAltitude(desiredAlt);
				break;
			case 'k':
				desiredAlt -= 0.25;
				drone->setAltitude(desiredAlt);
				break;*/
			case 'u':
				drone->setControlData(ControlChannel::THETA, 0.0f, -0.05f, 0.0, 0.0);
				break;
			case 'v':
				logConsume(NULL, "Vicon calibration in progress, please wait...");
				try {
					HTMatrix4 std = drone->calibrateVicon();
					drone->trim();
					logConsume(NULL, "Vicon calibration done. Transform standard deviation:");
					logConsume(NULL, std.toString().replace("\n", "|").replace("\t", " "));
				} catch(cvgException e) {
					logConsume(NULL, e.getMessage().c_str());
				}
				break;
			case 'o':
				drone->goToWaypoint(Vector3(0, 0, 2.5));
				drone->setSpeed(1.0);
				break;
			case '1':
				drone->goToWaypoint(Vector3(1.5, 1.5, 1.0));
				drone->setSpeed(1.0);
				break;
			case '2':
				drone->goToWaypoint(Vector3(1.5, -1.5, 1.0));
				drone->setSpeed(1.0);
				break;
			case '3':
				drone->goToWaypoint(Vector3(-1.5, -1.5, 1.0));
				drone->setSpeed(1.0);
				break;
			case '4':
				drone->goToWaypoint(Vector3(-1.5, 1.5, 1.0));
				drone->setSpeed(1.0);
				break;
			case '+':
				drone->accelerate(0.25);
				break;
			case '-':
				drone->accelerate(-0.25);
				break;
//			case 's':
//				drone->stopWhereYouAre();
//				break;
			case 'p':
				if (drone->getPositioningSource() == MyDrone::POSITIONING_VICON)
					drone->setPositioningSource(MyDrone::POSITIONING_ODOMETRY);
				else
					drone->setPositioningSource(MyDrone::POSITIONING_VICON);
/*			case 'r':
				if (drone->getTrajectory() == NULL) {
//					trajectory.generateXYCircle(8, Vector3(0, 0, 1.0), 1.5, 1.0);
					trajectory.clear();
					cvg_double angle = 0.0;
					cvg_uint numWaypoints = 60;
					cvg_double maxX = 1.8, maxY = 1.8, maxZ = 0.0;
					for (cvg_int i = 0; i < numWaypoints; i++) {
						Vector3 point(maxX * sin(2 * angle), maxY * sin(angle), 1.0 + maxZ * sin(angle));
						logConsume(NULL, cvgString("Waypoint - x: ") + point.x + " y:" + point.y + " z:" + point.z);
						trajectory.appendWaypoint(point, 1.0);
						angle += (2 * M_PI) / numWaypoints;
//						if (angle > 2 * M_PI) angle -= 2 * M_PI;
					}
					trajectory.setGrossPointBoxDimensions(Vector3(-1.0, 0.1, -1.0));
					drone->setTrajectory(&trajectory);
					drone->setSpeed(1.0);
				} else {
					drone->setTrajectory(NULL);
				}
				break;*/
		}
		usleep(100000);
	}
}

void ConsoleUI::writeLabel(cvg_uint y, cvg_uint x, const char *label, cvg_int valueLength, cvg_double value) {
	char str[32];
	sprintf(str, "%.2f", value);
	mvprintw(y, x, "%s: %s", label, str);
	cvg_int l = strlen(str);
	cvg_int labelLen = strlen(label);
	for (cvg_int i = 0; i < valueLength - l; i++) mvprintw(y, x + labelLen + 2 + l + i, " ");
}

void ConsoleUI::refreshInfo() {
	char fmStr[32];
	flyingModeToString(fmStr, drone->getControlMode());
	char dmStr[64];
	droneModeToString(dmStr, lastDroneInfo.feedbackData.droneMode);
	sprintf(&dmStr[strlen(dmStr)], " (native: 0x%x)", lastDroneInfo.feedbackData.nativeDroneState);
	cvg_bool connToDrone = drone->connToDrone();	// This must be outside the lock-unlock to avoid potential deadlock
	if (consoleMutex.lock()) {
		int y = 0;
		mvprintw(y, 0, "Comm. with proxy:");
		if (drone->connToProxy()) {
			Channel::Statistics cStats = drone->getChannelManager().getControlChannel().getSendStatistics();
			Channel::Statistics fStats = drone->getChannelManager().getFeedbackChannel().getRecvStatistics();
			Channel::Statistics vStats = drone->getChannelManager().getVideoChannel(0).getRecvStatistics();
			mvprintw(y, 18, "OK (c:%.1f|f:%.1f|v:%.1f)     ", cStats.throughput, fStats.throughput, vStats.throughput);
		} else {
			mvprintw(y, 18, "ERROR                         ");
		}
		mvprintw(y++, 50, "Comm. with drone: %s   ", connToDrone ? "OK" : "ERROR");
		y++;
		mvprintw(y, 0, "Control mode: %s      ", fmStr);
		mvprintw(y, 30, "Drone mode: %s      ", connToDrone ? dmStr : "??");
		mvprintw(y += 2, 0, "Battery: %.2f%%  ", lastDroneInfo.feedbackData.batteryLevel * 100);
		y += 2;
		cvg_int backupY = y;
		// Odometry
		mvprintw(y++, 0, "ODOMETRY");
		writeLabel(y, 0, "x", 7, 0.0);
		writeLabel(y, 11, "y", 7, 0.0);
		writeLabel(y++, 22, "z", 7, -lastDroneInfo.feedbackData.altitude);
		writeLabel(y, 0, "Y", 7, lastDroneInfo.feedbackData.yaw);
		writeLabel(y, 11, "P", 7, lastDroneInfo.feedbackData.pitch);
		writeLabel(y++, 22, "R", 7, lastDroneInfo.feedbackData.roll);
		writeLabel(y, 0, "Vf", 7, lastDroneInfo.feedbackData.speedX);
		writeLabel(y++, 11, "Vl", 7, lastDroneInfo.feedbackData.speedY);
//		mvprintw(y++, 25, "Vyaw: %.2f", lastDroneInfo.feedbackData.speedYaw);

		// Vicon
		y = backupY;
		MyDrone::ViconData vd;
		if (drone->getViconData(&vd)) {
			mvprintw(y++, 35, "VICON");
			Vector3 pos = vd.localTransform.getTranslation() * 1e-3;
			writeLabel(y, 35, "x", 7, pos.x);
			writeLabel(y, 46, "y", 7, pos.y);
			writeLabel(y++, 57, "z", 7, pos.z);
			Vector3 euler = vd.localTransform.getRotationMatrix().getEulerAnglesZYX();
			writeLabel(y, 35, "Y", 7, euler.z * 180.0 / M_PI);
			writeLabel(y, 46, "P", 7, euler.y * 180.0 / M_PI);
			writeLabel(y++, 57, "R", 7, euler.x * 180.0 / M_PI);
			writeLabel(y, 35, "Vf", 7, vd.localSpeed.x * 1e-3);
			writeLabel(y, 46, "Vl", 7, vd.localSpeed.y * 1e-3);
//			mvprintw(y++, 25, "Vyaw: %.2f", lastDroneInfo.feedbackData.speedYaw);
		}

		refresh();
		consoleMutex.unlock();
	}
}

void ConsoleUI::logConsume(DroneProxy::Logs::LogProducer *producer, const cvgString &str) {
	if (consoleMutex.lock()) {
		if (!isInit) {
			printf("%s\n", str.c_str());
		} else {
			// Break string if it's wider than the window
			int maxx, maxy;
			getmaxyx(logWin, maxy, maxx);
			maxx = ((maxx - 2) - 1) + 1;	// Substract border
			cvg_int numFrags = (cvg_int)ceil(str.length() / (cvg_double)maxx);
	//		if (consoleMutex.lock()) {
				for (cvg_int i = 0; i < numFrags - 1; i++) {
					wprintw(logWin, (" " + str.subString(i * maxx, maxx) + "\n").c_str());
				}
				wprintw(logWin, (" " + str.subString((numFrags - 1) * maxx) + "\n").c_str());
	//			consoleMutex.unlock();
	//		}
			box(logWin, 0, 0);
			wrefresh(logWin);

#ifdef LOGS_TO_FILE
			fprintf(logFile, "%f %s\n", timer.getElapsedSeconds(), str.c_str());
#endif
		}
		consoleMutex.unlock();
	}
}

void ConsoleUI::gotData(Channel *channel, void *data) {
	if (channel == &drone->getChannelManager().getFeedbackChannel()) {
		if (consoleMutex.lock()) {
			memcpy(&lastDroneInfo.feedbackData, data, sizeof(lastDroneInfo.feedbackData));
			consoleMutex.unlock();
		}
	}
}
