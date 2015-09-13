#include "player.hpp"

#include <limits>
#include <algorithm>
#include <cstring>

#include "shared/engine/math.hpp"
#include "shared/block_utils.hpp"

#include "world.hpp"

using namespace std;

const double Player::FLY_ACCELERATION = 1000;
const double Player::FLY_SPRINT_ACCELERATION = 5000;
const double Player::FLY_FRICTION = 0.8;
const double Player::GROUND_ACCELERATION = 25;
const double Player::GROUND_SPRINT_ACCELERATION = 50;
const double Player::GROUND_FRICTION = 0.25;
const double Player::AIR_ACCELERATION = 2;
const double Player::AIR_FRICTION = 0.005;
const double Player::AIR_ACCELERATION_PENALTY = 0.01;
const double Player::JUMP_SPEED = 260;

void Player::tick(bool isLocalPlayer) {
	// TODO don't let player enter unloaded chunk
	vec3i64 cp = getChunkPos();
	if (world->isChunkLoaded(cp) || isFlying) {
		calcVel();
		if(isFlying)
			ghost();
		else
			collide();
	}
}

void Player::collide() {
	vec3i64 newPos = pos;
	vec3d remVel = vel;
	double remDist = remVel.norm();
	while (remDist >= 0.5) {
		vec3i64 firstHitPos;
		int firstHitFaceDirs[3];
		int numFirstHitFaces = 0;
		double lowestDist = std::numeric_limits<double>::infinity();
		for (int i = 0; i < 12; i++) {
			vec3i corner(
				QUAD_CORNER_CYCLE[i % 4][0],
				QUAD_CORNER_CYCLE[i % 4][1],
				min(1, i / 4)
			);
			vec3i off(
				(corner[0] * 2 - 1) * RADIUS,
				(corner[1] * 2 - 1) * RADIUS,
				HEIGHT / 2 * (i / 4) - EYE_HEIGHT
			);
			vec3i64 start = newPos + off.cast<int64>();
			vec3i64 hitPos;
			int faceDirs[3];
			int numHitFaces = world->shootRay(start, remVel, remDist,
					corner, &hitPos, nullptr, faceDirs);
			if (numHitFaces > 0) {
				double dist = (hitPos - start).norm();
				if (dist < lowestDist) {
					lowestDist = dist;
					firstHitPos = hitPos - off.cast<int64>();
					memcpy(firstHitFaceDirs, faceDirs, 3 * sizeof(int));
					numFirstHitFaces = numHitFaces;
				}
			}
		}
		if (numFirstHitFaces == 0) {
			newPos[0] = newPos[0] + (int64)round(remVel[0]);
			newPos[1] = newPos[1] + (int64)round(remVel[1]);
			newPos[2] = newPos[2] + (int64)round(remVel[2]);
			break;
		}

		remVel -= (firstHitPos - newPos).cast<double>();

		for (int i = 0; i < numFirstHitFaces; i++) {
			remVel[DIR_DIMS[firstHitFaceDirs[i]]] = 0;
			vel[DIR_DIMS[firstHitFaceDirs[i]]] = 0;
		}

		newPos = firstHitPos;

		remDist = remVel.norm();
	}
	pos = newPos;
}

void Player::ghost() {
	pos[0] += (int64)round(vel[0]);
	pos[1] += (int64)round(vel[1]);
	pos[2] += (int64)round(vel[2]);
}

void Player::setOrientation(int yaw, int pitch) {
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

int Player::getYaw() const {
	return yaw;
}

int Player::getPitch() const {
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

	pos = vec3i64(0, 0, 30 * RESOLUTION);
	valid = true;
}

void Player::destroy() {
	world = nullptr;
	valid = false;
}

bool Player::isValid() const {
	return valid;
}

bool Player::getTargetedFace(vec3i64 *outBlock, int *outFaceDir) const {
	vec3d dir = getVectorFromAngles(yaw / 100.0f, pitch / 100.0f);
	vec3i64 hitBlock[3];
	int faceDir[3];
	int hit = world->shootRay(pos, dir, TARGET_RANGE * RESOLUTION,
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

void Player::applySnapshot(const PlayerSnapshot &snapshot, bool local) {
	pos = snapshot.pos;
	vel = snapshot.vel;
	isFlying = snapshot.isFlying;
	if (!local) {
		yaw = snapshot.yaw;
		pitch = snapshot.pitch;
		moveInput = snapshot.moveInput;
	}
}

PlayerSnapshot Player::makeSnapshot(int tick) const {
	return PlayerSnapshot{tick, pos, vel, (uint16) round(yaw * 100), (int16) round(pitch * 100), moveInput, isFlying};
}

void Player::calcVel() {
	vec3d inFac(0.0, 0.0, 0.0);
	bool right = (moveInput & MOVE_INPUT_FLAG_STRAFE_RIGHT) > 0;
	bool left = (moveInput & MOVE_INPUT_FLAG_STRAFE_LEFT) > 0;
	bool forward = (moveInput & MOVE_INPUT_FLAG_MOVE_FORWARD) > 0;
	bool backward = (moveInput & MOVE_INPUT_FLAG_MOVE_BACKWARD) > 0;
	bool up = (moveInput & MOVE_INPUT_FLAG_FLY_UP) > 0;
	bool down = (moveInput & MOVE_INPUT_FLAG_FLY_DOWN) > 0;
	bool sprint = (moveInput & MOVE_INPUT_FLAG_SPRINT) > 0;

	if (right && !left) {
		inFac[0] += sin(yaw * TAU / 36000.0f);
		inFac[1] -= cos(yaw * TAU / 36000.0f);
	} else if (left && !right) {
		inFac[0] -= sin(yaw * TAU / 36000.0f);
		inFac[1] += cos(yaw * TAU / 36000.0f);
	}

	if (forward && !backward) {
		inFac[0] += cos(yaw * TAU / 36000.0f);
		inFac[1] += sin(yaw * TAU / 36000.0f);
	} else if (backward && !forward) {
		inFac[0] -= cos(yaw * TAU / 36000.0f);
		inFac[1] -= sin(yaw * TAU / 36000.0f);
	}

	if (isFlying) {
		if (up && !down)
			inFac[2] += 1;
		else if (down && !up)
			inFac[2] -= 1;
	}

	double norm = inFac.norm();
	if (norm > 0) {
		inFac /= norm;
	}

	bool grounded = isGrounded();
	double acceleration;
	double friction;

	if (isFlying) {
		if (sprint)
			acceleration = FLY_SPRINT_ACCELERATION;
		else
			acceleration = FLY_ACCELERATION;
		friction = FLY_FRICTION;
	} else if (grounded) {
		if (sprint)
			acceleration = GROUND_SPRINT_ACCELERATION;
		else
			acceleration = GROUND_ACCELERATION;
		friction = GROUND_FRICTION;
	} else {
		acceleration = AIR_ACCELERATION * (1 - max(0.0, min(1.0, vec2d(inFac[0], inFac[1]) * vec2d(vel[0], vel[1]) * AIR_ACCELERATION_PENALTY)));
		friction = AIR_FRICTION;
	}

	if(!isFlying) {
		vel[2] += World::GRAVITY;
		if (grounded && (moveInput & MOVE_INPUT_FLAG_FLY_UP) > 0)
			vel[2] = JUMP_SPEED;
	}

	vel += inFac * acceleration - vel * friction;
}

bool Player::isGrounded() const {
	if (vel[2] > 0)
		return false;
	for (int i = 0; i < 4; i++) {
		vec3i off = vec3i(
			(QUAD_CORNER_CYCLE[i % 4][0] * 2 - 1) * RADIUS,
			(QUAD_CORNER_CYCLE[i % 4][1] * 2 - 1) * RADIUS,
			-EYE_HEIGHT - 1
		);
		vec3i64 corner = pos + off.cast<int64>();
		if (world->hasCollision(corner))
			return true;
	}
	return false;
}
