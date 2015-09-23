#ifndef WITHOUT_GL2

#include "gl2_debug_renderer.hpp"

#include "shared/engine/logging.hpp"
#include "shared/engine/math.hpp"

#include "gl2_renderer.hpp"

static logging::Logger logger("render");

GL2DebugRenderer::GL2DebugRenderer(Client *client, GL2Renderer *renderer) :
	client(client), renderer(renderer)
{
	LOG_DEBUG(logger) << "Loading font";
	font = new FTGLTextureFont("res/DejaVuSansMono.ttf");
	if (font) {
		font->FaceSize(16);
		font->CharMap(ft_encoding_unicode);
	} else {
		LOG_ERROR(logger) << "Could not open 'res/DejaVuSansMono.ttf'";
	}
}

GL2DebugRenderer::~GL2DebugRenderer() {
	delete font;
}

void GL2DebugRenderer::render() {
	if (!client->isDebugOn() || client->getStateId() != Client::StateId::PLAYING)
		return;

	renderDebug();

	if (client->getStopwatch())
		renderPerformance();
}

void GL2DebugRenderer::renderDebug() {
	const Character &character = client->getLocalCharacter();

	vec3i64 characterPos = character.getPos();
	vec3d characterVel = character.getVel();
	//uint32 windowFlags = SDL_GetWindowFlags(window);

	glDisable(GL_TEXTURE_2D);
	glPushMatrix();
	glColor3f(1.0f, 1.0f, 1.0f);
	glTranslatef(-client->getGraphics()->getDrawWidth() / 2.0f + 3.0f, client->getGraphics()->getDrawHeight() / 2.0f, 0.0f);
	char buffer[1024];
	#define RENDER_LINE(...) sprintf(buffer, __VA_ARGS__);\
			glTranslatef(0.0f, -16.0f, 0.0f);\
			font->Render(buffer)

	//RENDER_LINE("fps: %d", fpsSum);
	//RENDER_LINE("new faces: %d", newFaces);
	//RENDER_LINE("faces: %d", faces);
	RENDER_LINE("x: %" PRId64 "(%" PRId64 ")", characterPos[0], (int64) floor(characterPos[0] / (double) RESOLUTION));
	RENDER_LINE("y: %" PRId64 " (%" PRId64 ")", characterPos[1], (int64) floor(characterPos[1] / (double) RESOLUTION));
	RENDER_LINE("z: %" PRId64 " (%" PRId64 ")", characterPos[2],
			(int64) floor((characterPos[2] - Character::EYE_HEIGHT - 1) / (double) RESOLUTION));
	RENDER_LINE("yaw:   %6.1f", character.getYaw() / 100.0f);
	RENDER_LINE("pitch: %6.1f", character.getPitch() / 100.0f);
	RENDER_LINE("xvel: %8.1f", characterVel[0]);
	RENDER_LINE("yvel: %8.1f", characterVel[1]);
	RENDER_LINE("zvel: %8.1f", characterVel[2]);
	//RENDER_LINE("chunks loaded: %" PRIuPTR "", client->getWorld()->getNumChunks());
	//RENDER_LINE("chunks visible: %d", visibleChunks);
	//RENDER_LINE("faces visible: %d", visibleFaces);
	//RENDER_LINE("block: %d", character.getBlock());*/

	glPopMatrix();
}

void GL2DebugRenderer::renderPerformance() {
	const char *rel_names[] = {
		"WOT", // world tick
		"CRT", // chunk renderer tick
		"BCH", // build chunk
		"CRR", // chunk renderer render
		"IBQ", // iterate render queue
		"VS", // visibility search
		"FSH", // glFinish
		"FLP", // flip
		"SYN", // synchronize
		"ALL"
	};

	vec3f rel_colors[] {
		{0.6f, 0.6f, 1.0f},
		{0.0f, 0.0f, 0.8f},
		{0.6f, 0.0f, 0.8f},
		{0.0f, 0.6f, 0.8f},
		{0.4f, 0.4f, 0.8f},
		{0.7f, 0.7f, 0.0f},
		{0.8f, 0.8f, 0.3f},
		{0.0f, 0.8f, 0.0f},
		{0.0f, 0.4f, 0.0f},
		{0.7f, 0.1f, 0.7f},
		{0.7f, 0.7f, 0.4f},
		{0.2f, 0.6f, 0.6f},
		{0.8f, 0.0f, 0.0f},
	};

	float rel = 0.0;
	float cum_rels[CLOCK_ID_NUM + 1];
	cum_rels[0] = 0;
	float center_positions[CLOCK_ID_NUM];

	glPushMatrix();
	glTranslatef(+client->getGraphics()->getDrawWidth() / 2.0f, -client->getGraphics()->getDrawHeight() / 2.0f, 0.0f);
	glScalef(10.0, (float) client->getGraphics()->getDrawHeight(), 1.0);
	glTranslatef(-1, 0, 0);
	glBegin(GL_QUADS);
	for (uint i = 0; i < CLOCK_ID_NUM; ++i) {
		glColor3f(rel_colors[i][0], rel_colors[i][1], rel_colors[i][2]);
		glVertex2f(0, rel);
		glVertex2f(1, rel);
		rel += client->getStopwatch()->getRel(i);
		cum_rels[i + 1] = rel;
		center_positions[i] = (cum_rels[i] + cum_rels[i + 1]) / 2.0f;
		glVertex2f(1, rel);
		glVertex2f(0, rel);
	}
	glEnd();
	glPopMatrix();

	static const float REL_THRESHOLD = 0.001f;
	uint labeled_ids[CLOCK_ID_NUM];
	int num_displayed_labels = 0;
	for (int i = 0; i < CLOCK_ID_NUM; ++i) {
		if (client->getStopwatch()->getRel(i) > REL_THRESHOLD)
			labeled_ids[num_displayed_labels++] = i;
	}

	float used_positions[CLOCK_ID_NUM + 2];
	used_positions[0] = 0.0;
	for (int i = 0; i < num_displayed_labels; ++i) {
		int id = labeled_ids[i];
		used_positions[i + 1] = center_positions[id];
	}
	used_positions[num_displayed_labels + 1] = 1.0;

	for (int iteration = 0; iteration < 3; ++iteration)
	for (int i = 1; i < num_displayed_labels + 1; ++i) {
		float d1 = used_positions[i] - used_positions[i - 1];
		float d2 = used_positions[i + 1] - used_positions[i];
		float diff = 2e-4f * (1.0f / d1 - 1.0f / d2);
		used_positions[i] += clamp(diff, -0.02f, 0.02f);
	}

	glPushMatrix();
	glTranslatef(+client->getGraphics()->getDrawWidth() / 2.0f, -client->getGraphics()->getDrawHeight() / 2.0f, 0);
	glTranslatef(-15, 0, 0);
	glRotatef(90, 0, 0, 1);
	for (int i = 0; i < num_displayed_labels; ++i) {
		int id = labeled_ids[i];
		glPushMatrix();
		glTranslatef(used_positions[i + 1] * client->getGraphics()->getDrawHeight() - 14, 0, 0);
		char buffer[1024];
		sprintf(buffer, "%s", rel_names[id]);
		auto color = rel_colors[id];
		glColor3f(color[0], color[1], color[2]);
		font->Render(buffer);
		glPopMatrix();
	}
	glPopMatrix();
}

#endif // WITHOUT_GL2
