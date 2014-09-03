#include "player.hpp"
#include <cmath>
#include <limits>
#include "util.hpp"
#include "world.hpp"
#include <algorithm>
#include <cstring>

using namespace std;

const double Player::FLY_SPEED = 500;
const double Player::WALK_SPEED = 100;
const double Player::AIR_SPEED = 40;
const double Player::AIR_ACCELERATION = 4;
const double Player::AIR_FRICTION = 0.005;
const double Player::JUMP_SPEED = 200;

void Player::tick(int tick, bool isLocalPlayer) {

	// TODO predict
	if (hasSnapshot) {
		validPosMonitor.startWrite();
		pos = snapshot.pos;
		validPosMonitor.finishWrite();
		vel = snapshot.vel;
		if (!isLocalPlayer) {
			yaw = snapshot.yaw;
			pitch = snapshot.pitch;
			moveInput = snapshot.moveInput;
		}
	}

	if (isFlying) {
		fly();
	} else {
		// player physics
		vec3i64 cp = getChunkPos();
		// TODO don't let player enter chunk
		if (world->getChunks().find(cp) != world->getChunks().end()) {
			walk();
			move();
		}
	}

	// printf("%f\n", pitch);
}

void Player::move() {
	using namespace vec_auto_cast;
	vec3i64 newPos = pos;
	vec3d remVel = vel;
	double remDist = remVel.norm();
	while (remDist >= 0.5) {
		vec3d firstHitPos;
		int firstHitFaceDirs[3];
		int numFirstHitFaces = 0;
		double lowestDist = std::numeric_limits<double>::infinity();
		for (int i = 0; i < 12; i++) {
			vec3i corner(
				QUAD_CYCLE_2D[i % 4][0],
				QUAD_CYCLE_2D[i % 4][1],
				min(1, i / 4)
			);
			vec3i off(
				(corner[0] * 2 - 1) * RADIUS,
				(corner[1] * 2 - 1) * RADIUS,
				HEIGHT / 2 * (i / 4) - EYE_HEIGHT
			);
			vec3i64 start = newPos + off.cast<int64>();
			vec3d hitPos;
			int faceDirs[3];
			int numHitFaces = world->shootRay(start, remVel, remDist,
					corner, &hitPos, nullptr, faceDirs);
			if (numHitFaces > 0) {
				double dist = (hitPos - start).norm();
				if (dist < lowestDist) {
					lowestDist = dist;
					firstHitPos = hitPos - off;
					memcpy(firstHitFaceDirs, faceDirs, 3 * sizeof(int));
					numFirstHitFaces = numHitFaces;
				}
			}
		}
		if (numFirstHitFaces == 0) {
			newPos[0] = floor(newPos[0] + remVel[0]);
			newPos[1] = floor(newPos[1] + remVel[1]);
			newPos[2] = floor(newPos[2] + remVel[2]);
			break;
		}

		remVel -= firstHitPos - newPos;

		for (int i = 0; i < numFirstHitFaces; i++) {
			remVel[DIR_DIMS[firstHitFaceDirs[i]]] = 0;
			vel[DIR_DIMS[firstHitFaceDirs[i]]] = 0;
		}

		newPos[0] = floor(firstHitPos[0]);
		newPos[1] = floor(firstHitPos[1]);
		newPos[2] = floor(firstHitPos[2]);

		remDist = remVel.norm();
	}
	validPosMonitor.startWrite();
	pos = newPos;
	validPosMonitor.finishWrite();
}

void Player::setOrientation(double yaw, double pitch) {
	this->yaw = yaw;
	this->pitch = pitch;
}

void Player::setFly(bool isFlying) {
	this->isFlying = isFlying;
}

void Player::setMoveInput(int moveInput) {
	this->moveInput = moveInput;
}

bool Player::getFly() const {
	return isFlying;
}

vec3i64 Player::getPos() const {
	return pos;
}

vec3d Player::getVel() const {
	return vel;
}

double Player::getYaw() const {
	return yaw;
}

double Player::getPitch() const {
	return pitch;
}

int Player::getMoveInput() const {
	return moveInput;
}

vec3i64 Player::getChunkPos() const {
	return bc2cc(wc2bc(pos));
}

void Player::create(World *world) {
	this->world = world;

	vel = vec3d(0, 0, 0);
	yaw = 0;
	pitch = 0;
	isFlying = false;
	moveInput = 0;

	validPosMonitor.startWrite();
	pos = vec3i64(0, 0, 30 * RESOLUTION);
	valid = true;
	validPosMonitor.finishWrite();
}

void Player::destroy() {
	world = nullptr;
	validPosMonitor.startWrite();
	valid = false;
	validPosMonitor.finishWrite();
}

bool Player::isValid() const {
	return valid;
}

bool Player::getTargetedFace(vec3i64 *outBlock, int *outFaceDir) const {
	vec3d dir = getVectorFromAngles(yaw, pitch);
	vec3i64 hitBlock[3];
	int faceDir[3];
	int hit = world->shootRay(pos, dir, 100 * RESOLUTION,
			vec3i(0, 0, 0), nullptr, hitBlock, faceDir);
	if (hit > 0) {
		if (outBlock != nullptr && outFaceDir != nullptr) {
			*outBlock = hitBlock[0];
			*outFaceDir = faceDir[0];
		}
		return true;
	}
	return false;
}

void Player::setSnapshot(const PlayerSnapshot &snapshot) {
	this->snapshot = snapshot;
	hasSnapshot = true;
}

PlayerSnapshot Player::makeSnapshot(int tick) const {
	return PlayerSnapshot{tick, isFlying, pos, vel, yaw, pitch, moveInput};
}

Monitor &Player::getValidPosMonitor() {
	return validPosMonitor;
}

void Player::fly() {
	vec3i64 newPos = pos;
	if ((moveInput & MOVE_INPUT_FLAG_STRAFE_RIGHT) > 0) {
		newPos[0] += sin(yaw * TAU / 360) * FLY_SPEED;
		newPos[1] -= cos(yaw * TAU / 360) * FLY_SPEED;
	} else if ((moveInput & MOVE_INPUT_FLAG_STRAFE_LEFT) > 0) {
		newPos[0] -= sin(yaw * TAU / 360) * FLY_SPEED;
		newPos[1] += cos(yaw * TAU / 360) * FLY_SPEED;
	}

	if ((moveInput & MOVE_INPUT_FLAG_FLY_UP) > 0) {
		newPos[2] += FLY_SPEED;
	} else if ((moveInput & MOVE_INPUT_FLAG_FLY_DOWN) > 0) {
		newPos[2] -= FLY_SPEED;
	}

	if ((moveInput & MOVE_INPUT_FLAG_MOVE_FORWARD) > 0) {
		newPos[0] += cos(yaw * TAU / 360) * FLY_SPEED;
		newPos[1] += sin(yaw * TAU / 360) * FLY_SPEED;
	} else if ((moveInput & MOVE_INPUT_FLAG_MOVE_BACKWARD) > 0) {
		newPos[0] -= cos(yaw * TAU / 360) * FLY_SPEED;
		newPos[1] -= sin(yaw * TAU / 360) * FLY_SPEED;
	}

	validPosMonitor.startWrite();
	pos = newPos;
	validPosMonitor.finishWrite();
}

void Player::walk() {
	vec2d walkFac(0.0, 0.0);
	if ((moveInput & MOVE_INPUT_FLAG_STRAFE_RIGHT) > 0) {
		walkFac[0] += sin(yaw * TAU / 360);
		walkFac[1] -= cos(yaw * TAU / 360);
	} else if ((moveInput & MOVE_INPUT_FLAG_STRAFE_LEFT) > 0) {
		walkFac[0] -= sin(yaw * TAU / 360);
		walkFac[1] += cos(yaw * TAU / 360);
	}

	if ((moveInput & MOVE_INPUT_FLAG_MOVE_FORWARD) > 0) {
		walkFac[0] += cos(yaw * TAU / 360);
		walkFac[1] += sin(yaw * TAU / 360);
	} else if ((moveInput & MOVE_INPUT_FLAG_MOVE_BACKWARD) > 0) {
		walkFac[0] -= cos(yaw * TAU / 360);
		walkFac[1] -= sin(yaw * TAU / 360);
	}

	double norm = walkFac.norm();
	if (norm > 0) {
		walkFac /= norm;
	}

	vec2d xyVel(vel[0], vel[1]);
	vel[2] += World::GRAVITY;
	bool grounded = isGrounded();
	if (grounded) {
		if ((moveInput & MOVE_INPUT_FLAG_FLY_UP) > 0) {
			vel[2] = JUMP_SPEED;
		}
		xyVel = walkFac * WALK_SPEED;
	} else if (norm > 0) {
		double lastNorm = xyVel.norm();
		xyVel += walkFac * AIR_ACCELERATION;
		double newNorm = xyVel.norm();
		if (newNorm > AIR_SPEED && lastNorm < AIR_SPEED) {
			xyVel *= AIR_SPEED / newNorm;
		} else if (newNorm > AIR_SPEED && newNorm > lastNorm) {
			xyVel *= lastNorm / newNorm;
		}
	} else {
		xyVel *= 1 - AIR_FRICTION;
	}

	vel[0] = xyVel[0];
	vel[1] = xyVel[1];
}

bool Player::isGrounded() const {
	using namespace vec_auto_cast;
	if (vel[2] > 0)
		return false;
	for (int i = 0; i < 4; i++) {
		vec3i off = vec3i(
			(QUAD_CYCLE_2D[i % 4][0] * 2 - 1) * RADIUS,
			(QUAD_CYCLE_2D[i % 4][1] * 2 - 1) * RADIUS,
			-EYE_HEIGHT - 1
		);
		vec3i64 corner = pos + off;
		if (world->hasCollision(corner))
			return true;
	}
	return false;
}
