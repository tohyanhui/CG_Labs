#include "assignment5.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/ShaderProgramManager.hpp"

#include <imgui.h>
#include <tinyfiledialogs.h>

#include <clocale>
#include <stdexcept>
#include <array>
#include <cmath>
#include <random>
#include <algorithm>

#include "parametric_shapes.hpp"
#include "core/node.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
			static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
			0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("Torus Ride", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

void
edaf80::Assignment5::run()
{
	// Set up the camera
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 6.0f));
	mCamera.mMouseSensitivity = glm::vec2(0.003f);
	mCamera.mMovementSpeed = glm::vec3(3.0f); // 3 m/s => 10.8 km/h

	glm::vec3 player_position(0.0f, 0.0f, 6.0f);

	// Create the shader programs
	ShaderProgramManager program_manager;
	GLuint fallback_shader = 0u;
	program_manager.CreateAndRegisterProgram("Fallback",
											 { { ShaderType::vertex, "common/fallback.vert" },
											   { ShaderType::fragment, "common/fallback.frag" } },
											 fallback_shader);
	if (fallback_shader == 0u) {
		LogError("Failed to load fallback shader");
		return;
	}

	GLuint skybox_shader = 0u;
	program_manager.CreateAndRegisterProgram(
		"Skybox",
		{
			{ ShaderType::vertex, "EDAF80/skybox.vert" },
			{ ShaderType::fragment, "EDAF80/skybox.frag" }
		},
		skybox_shader
	);

	if (skybox_shader == 0u) {
		LogError("Failed to load skybox shader");
		return;
	}

	GLuint ring_shader = 0u;
	program_manager.CreateAndRegisterProgram(
		"Rings",
		{
			{ ShaderType::vertex, "EDAF80/water_ring.vert" },
			{ ShaderType::fragment, "EDAF80/water_ring.frag" }
		},
		ring_shader
	);

	if (ring_shader == 0u) {
		LogError("Failed to load ring shader");
		return;
	}

	GLuint player_shader = 0u;
	program_manager.CreateAndRegisterProgram(
		"Player",
		{
			{ ShaderType::vertex, "EDAF80/phong.vert" },
			{ ShaderType::fragment, "EDAF80/phong.frag" }
		},
		player_shader
	);
	if (player_shader == 0u) {
		LogError("Failed to load player shader");
		return;
	}

	auto player_shape = parametric_shapes::createSphere(1.0f, 32u, 16u);
	if (player_shape.vao == 0u) {
		LogError("Failed to create player mesh");
		return;
	}

	// Body, belly, dorsal fin, side fins, tail stem, tail lobes and eyes.
	constexpr std::size_t player_part_count = 10;
	std::array<Node, player_part_count> player_parts;
	std::array<glm::vec3, player_part_count> const part_positions = {
		glm::vec3(0.0f, 0.0f, -0.10f),
		glm::vec3(0.0f, -0.13f, -0.20f),
		glm::vec3(0.0f, 0.30f, 0.05f),
		glm::vec3(-0.35f, -0.12f, 0.0f),
		glm::vec3(0.35f, -0.12f, 0.0f),
		glm::vec3(0.0f, 0.0f, 0.70f),
		glm::vec3(0.0f, 0.20f, 1.0f),
		glm::vec3(0.0f, -0.16f, 1.0f),
		glm::vec3(-0.22f, 0.08f, -0.60f),
		glm::vec3(0.22f, 0.08f, -0.60f)
	};
	std::array<glm::vec3, player_part_count> const part_scales = {
		glm::vec3(0.30f, 0.25f, 0.85f),
		glm::vec3(0.25f, 0.14f, 0.68f),
		glm::vec3(0.045f, 0.30f, 0.22f),
		glm::vec3(0.36f, 0.045f, 0.20f),
		glm::vec3(0.36f, 0.045f, 0.20f),
		glm::vec3(0.12f, 0.12f, 0.35f),
		glm::vec3(0.045f, 0.30f, 0.16f),
		glm::vec3(0.045f, 0.22f, 0.14f),
		glm::vec3(0.045f),
		glm::vec3(0.045f)
	};

	for (std::size_t i = 0; i < player_part_count; ++i) {
		player_parts[i].set_geometry(player_shape);
		bonobo::material_data material{};
		if (i >= 8)
			material.diffuse = glm::vec3(0.015f);
		else if (i == 1)
			material.diffuse = glm::vec3(0.85f, 0.88f, 0.90f);
		else
			material.diffuse = glm::vec3(0.25f, 0.36f, 0.43f);
		material.ambient = material.diffuse * 0.20f;
		material.specular = glm::vec3(0.25f);
		material.shininess = 32.0f;
		player_parts[i].set_material_constants(material);
		player_parts[i].set_program(&player_shader, [&](GLuint program) {
			auto const camera = mCamera.mWorld.GetTranslation();
			auto const light = player_position + glm::vec3(-5.0f, 8.0f, -4.0f);
			glUniform3fv(glGetUniformLocation(program, "camera_position"),
						 1, glm::value_ptr(camera));
			glUniform3fv(glGetUniformLocation(program, "light_position"),
						 1, glm::value_ptr(light));
			glUniform1i(glGetUniformLocation(program, "has_diffuse_texture"), 0);
			glUniform1i(glGetUniformLocation(program, "has_specular_texture"), 0);
			glUniform1i(glGetUniformLocation(program, "has_normal_texture"), 0);
			glUniform1i(glGetUniformLocation(program, "use_normal_mapping"), 0);
		});
	}

	GLuint cubemap = bonobo::loadTextureCubeMap(
		config::resources_path("cubemaps/NissiBeach2/posx.jpg"),
		config::resources_path("cubemaps/NissiBeach2/negx.jpg"),
		config::resources_path("cubemaps/NissiBeach2/posy.jpg"),
		config::resources_path("cubemaps/NissiBeach2/negy.jpg"),
		config::resources_path("cubemaps/NissiBeach2/posz.jpg"),
		config::resources_path("cubemaps/NissiBeach2/negz.jpg")
	);

	GLuint normal_texture = bonobo::loadTexture2D(
		config::resources_path("textures/waves.png")
	);

	if (cubemap == 0u || normal_texture == 0u) {
		LogError("Failed to load water-ring textures");
		return;
	}

	auto skybox_shape = parametric_shapes::createSphere(
		200.0f, 64u, 32u
	);

	if (skybox_shape.vao == 0u) {
		LogError("Failed to create skybox mesh");
		return;
	}

	Node skybox;
	skybox.set_geometry(skybox_shape);
	skybox.add_texture("cubemap", cubemap, GL_TEXTURE_CUBE_MAP);
	skybox.set_program(&skybox_shader);

	auto ring_shape = parametric_shapes::createTorus(
		2.0f, 0.2f, 128u, 32u
	);

	if (ring_shape.vao == 0u) {
		LogError("Failed to create torus mesh");
		return;
	}

	constexpr std::size_t ring_count = 10;
	std::array<Node, ring_count> rings;
	std::array<glm::mat4, ring_count> ring_transforms;
	std::array<glm::vec3, ring_count> ring_positions;
	std::mt19937 random_engine(std::random_device{}());
	std::uniform_real_distribution<float> random_x(-1.5f, 1.5f);
	std::uniform_real_distribution<float> random_y(-1.0f, 1.0f);
	float next_ring_z = 0.0f;

	auto make_ring_transform = [&](std::size_t i, float time) {
		float const phase = static_cast<float>(i) * 0.8f;
		auto position = ring_positions[i];
		position.x += 0.8f * std::sin(time + phase);
		position.y += 0.5f * std::sin(time * 0.7f + phase);
		auto transform = glm::translate(glm::mat4(1.0f), position);
		transform = glm::rotate(transform, 0.25f * std::sin(time + phase),
								glm::vec3(0.0f, 1.0f, 0.0f));
		return glm::rotate(transform, glm::half_pi<float>(),
						   glm::vec3(1.0f, 0.0f, 0.0f));
	};
	auto reset_course = [&]() {
		for (std::size_t i = 0; i < ring_count; ++i) {
			ring_positions[i] = glm::vec3(
				i == 0 ? 0.0f : random_x(random_engine),
				i == 0 ? 0.0f : random_y(random_engine),
				-8.0f * static_cast<float>(i)
			);
			ring_transforms[i] = make_ring_transform(i, 0.0f);
		}
		next_ring_z = -8.0f * static_cast<float>(ring_count);
	};
	reset_course();


	for (std::size_t i = 0; i < ring_count; ++i) {
		rings[i].set_geometry(ring_shape);
		rings[i].add_texture("cubemap", cubemap, GL_TEXTURE_CUBE_MAP);
		rings[i].add_texture("normal_texture", normal_texture, GL_TEXTURE_2D);

		rings[i].set_program(&ring_shader, [&](GLuint program) {
			auto const position = mCamera.mWorld.GetTranslation();

			glUniform3fv(
				glGetUniformLocation(program, "camera_position"),
				1, glm::value_ptr(position)
			);
		});

	}

	glClearDepthf(1.0f);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glEnable(GL_DEPTH_TEST);


	auto lastTime = std::chrono::high_resolution_clock::now();
	float animation_time = 0.0f;
	float player_roll = 0.0f;
	float player_pitch = 0.0f;
	float feedback_time = 0.0f;
	bool feedback_passed = false;

	std::array<bool, ring_count> ring_checked{};
	int score = 0;
	int missed = 0;
	int lives = 3;
	float elapsed_time = 0.0f;
	bool game_finished = false;
	bool game_started = false;
	
	bool show_logs = false;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::high_resolution_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();
		auto const previous_player_position = player_position;
		auto const previous_ring_transforms = ring_transforms;

		
		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
								   "An error occurred while reloading shader programs; see the logs for details.\n"
								   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
								   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);


		// Retrieve the actual framebuffer size: for HiDPI monitors,
		// you might end up with a framebuffer larger than what you
		// actually asked for. For example, if you ask for a 1920x1080
		// framebuffer, you might get a 3840x2160 one instead.
		// Also it might change as the user drags the window between
		// monitors with different DPIs, or if the fullscreen status is
		// being toggled.
		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);


		float const dt = std::chrono::duration<float>(deltaTimeUs).count();
		feedback_time = std::max(0.0f, feedback_time - dt);
		glm::vec3 steering(0.0f);
		if (game_started && !game_finished) {
			player_position.z -= 6.0f * dt;
			if (!io.WantCaptureKeyboard) {
				if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) steering.y += 1.0f;
				if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) steering.y -= 1.0f;
				if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) steering.x -= 1.0f;
				if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) steering.x += 1.0f;
				if (glm::length(steering) > 0.0f)
					player_position += glm::normalize(steering) * 4.0f * dt;
			}
		}
		// Smoothly bank into turns and pitch when climbing or descending.
		float const lean_blend = 1.0f - std::exp(-8.0f * dt);
		player_roll += (-steering.x * 0.30f - player_roll) * lean_blend;
		player_pitch += (steering.y * 0.18f - player_pitch) * lean_blend;
		mCamera.mWorld.SetTranslate(player_position + glm::vec3(0.0f, 1.2f, 4.0f));
		mCamera.mWorld.LookAt(player_position + glm::vec3(0.0f, 0.0f, -3.0f));

		if (game_started && !game_finished) {
			animation_time += dt;
			elapsed_time += dt;
		}
		for (std::size_t i = 0; i < ring_count; ++i)
			ring_transforms[i] = make_ring_transform(i, animation_time);



		for (std::size_t i = 0; i < ring_count; ++i) {
			if (!game_started || game_finished || ring_checked[i])
				continue;

			// Express player positions in the ring's local coordinates.
			auto const previous_local = glm::vec3(
				glm::inverse(previous_ring_transforms[i]) *
				glm::vec4(previous_player_position, 1.0f)
			);

			auto const current_local = glm::vec3(
				glm::inverse(ring_transforms[i]) *
				glm::vec4(player_position, 1.0f)
			);

			// Your torus lies in local XZ, so its plane is local Y = 0.
			if (previous_local.y > 0.0f && current_local.y <= 0.0f) {
				float const fraction =
					previous_local.y / (previous_local.y - current_local.y);

				auto const crossing = glm::mix(
					previous_local, current_local, fraction
				);

				float const distance_from_centre = glm::length(
					glm::vec2(crossing.x, crossing.z)
				);

				// Hole radius = major radius - tube radius = 2.0 - 0.2.
				if (distance_from_centre < 1.8f) {
					++score;
					feedback_passed = true;
					feedback_time = 0.45f;
				} else {
					feedback_passed = false;
					feedback_time = 0.45f;
					++missed;
					--lives;
				}

				ring_checked[i] = true;
			}
			// A tilted plane may be crossed far off course or swept by a ring.
			// Ensure a ring left behind eventually counts as a miss.
			if (!ring_checked[i] && player_position.z < ring_positions[i].z - 1.0f) {
				feedback_passed = false;
				feedback_time = 0.45f;
				++missed;
				--lives;
				ring_checked[i] = true;
			}
			if (lives <= 0) {
				game_finished = true;
				break;
			}
		}

		// Recycle resolved rings beyond the far end of the course.
		if (game_started && !game_finished) {
			for (std::size_t i = 0; i < ring_count; ++i) {
				if (!ring_checked[i])
					continue;
				ring_positions[i] = glm::vec3(
					random_x(random_engine), random_y(random_engine), next_ring_z
				);
				next_ring_z -= 8.0f;
				ring_transforms[i] = make_ring_transform(i, animation_time);
				ring_checked[i] = false;
			}
		}

		// Keep coordinates small during long endless runs.
		if (player_position.z < -512.0f) {
			player_position.z += 512.0f;
			next_ring_z += 512.0f;
			for (std::size_t i = 0; i < ring_count; ++i) {
				ring_positions[i].z += 512.0f;
				ring_transforms[i] = make_ring_transform(i, animation_time);
			}
			mCamera.mWorld.SetTranslate(player_position + glm::vec3(0.0f, 1.2f, 4.0f));
			mCamera.mWorld.LookAt(player_position + glm::vec3(0.0f, 0.0f, -3.0f));
		}

		mWindowManager.NewImGuiFrame();
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);


		if (!shader_reload_failed) {
			auto const view_projection = mCamera.GetWorldToClipMatrix();
			auto player_transform = glm::translate(glm::mat4(1.0f), player_position);
			player_transform = glm::rotate(player_transform, player_roll,
											glm::vec3(0.0f, 0.0f, 1.0f));
			player_transform = glm::rotate(player_transform, player_pitch,
											glm::vec3(1.0f, 0.0f, 0.0f));
			float const tail_angle = game_started
				? 0.30f * std::sin(animation_time * 7.0f) : 0.0f;
			for (std::size_t i = 0; i < player_part_count; ++i) {
				auto part_parent = player_transform;
				if (i >= 5 && i <= 7) {
					// Pivot the tail stem and both lobes together at its base.
					glm::vec3 const pivot(0.0f, 0.0f, 0.50f);
					part_parent = glm::translate(part_parent, pivot);
					part_parent = glm::rotate(part_parent, tail_angle,
											   glm::vec3(0.0f, 1.0f, 0.0f));
					part_parent = glm::translate(part_parent, -pivot);
				}
				auto transform = glm::translate(part_parent, part_positions[i]);
				transform = glm::scale(transform, part_scales[i]);
				player_parts[i].render(view_projection, transform);
			}


			// Keep the environment centred on the camera.
			skybox.get_transform().SetTranslate(
				mCamera.mWorld.GetTranslation()
			);

			// Draw the background without writing to the depth buffer.
			GLboolean const culling_enabled = glIsEnabled(GL_CULL_FACE);
			glDisable(GL_CULL_FACE);
			glDepthMask(GL_FALSE);

			skybox.render(view_projection);

			glDepthMask(GL_TRUE);
			if (culling_enabled)
				glEnable(GL_CULL_FACE);

			glUseProgram(ring_shader);
			glUniform1f(
				glGetUniformLocation(ring_shader, "elapsed_time_s"),
				animation_time
			);
			glUseProgram(0u);

			for (std::size_t i = 0; i < ring_count; ++i) {
				if (ring_checked[i])
					continue;
				rings[i].render(view_projection, ring_transforms[i]);
			}
		}


		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		//
		// Todo: If you want a custom ImGUI window, you can set it up
		//       here
		//
		bool const opened = ImGui::Begin("Torus Ride", nullptr, ImGuiWindowFlags_None);
		if (opened) {
			ImGui::TextWrapped("Swim through an endless course of random rings. Each miss costs a life.");
			ImGui::Text("W/S: up/down   A/D: left/right");
			ImGui::Text("Forward movement is automatic.");
			if (!game_started) {
				ImGui::Text("Ready?");
				if (ImGui::Button("Start"))
					game_started = true;
			}
			ImGui::Separator();
			ImGui::Text("Rings passed: %d", score);
			ImGui::Text("Lives: %d / 3", lives);
			ImGui::Text("Missed: %d", missed);
			ImGui::Text("Time: %.1f seconds", elapsed_time);

			if (game_finished) {
				ImGui::Separator();
				ImGui::Text("Game over!");
				ImGui::Text("Final score: %d rings", score);
			}

			if (ImGui::Button("Restart")) {
				score = 0;
				missed = 0;
				lives = 3;
				reset_course();
				elapsed_time = 0.0f;
				game_finished = false;
				game_started = false;
				animation_time = 0.0f;
				player_roll = 0.0f;
				player_pitch = 0.0f;
				feedback_time = 0.0f;
				ring_checked.fill(false);

				player_position = glm::vec3(0.0f, 0.0f, 6.0f);
				mCamera.mWorld.SetTranslate(player_position + glm::vec3(0.0f, 1.2f, 4.0f));
				mCamera.mWorld.LookAt(player_position + glm::vec3(0.0f, 0.0f, -3.0f));
			}

			ImGui::Separator();

			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		// Transparent overlay: a subtle tint and stronger coloured border.
		if (show_gui && feedback_time > 0.0f) {
			ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
			ImGui::SetNextWindowSize(io.DisplaySize);
			ImGui::SetNextWindowBgAlpha(0.0f);
			ImGui::Begin("##Ring feedback", nullptr,
				ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoInputs |
				ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
			float const fade = feedback_time / 0.45f;
			ImVec4 const colour = feedback_passed
				? ImVec4(0.10f, 1.0f, 0.30f, 0.10f * fade)
				: ImVec4(1.0f, 0.10f, 0.10f, 0.14f * fade);
			auto* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(ImVec2(0.0f, 0.0f), io.DisplaySize,
									 ImGui::ColorConvertFloat4ToU32(colour));
			ImVec4 border = colour;
			border.w = 0.65f * fade;
			auto const border_colour = ImGui::ColorConvertFloat4ToU32(border);
			float const w = io.DisplaySize.x;
			float const h = io.DisplaySize.y;
			draw_list->AddRectFilled(ImVec2(0, 0), ImVec2(w, 8), border_colour);
			draw_list->AddRectFilled(ImVec2(0, h - 8), ImVec2(w, h), border_colour);
			draw_list->AddRectFilled(ImVec2(0, 0), ImVec2(8, h), border_colour);
			draw_list->AddRectFilled(ImVec2(w - 8, 0), ImVec2(w, h), border_colour);
			ImGui::End();
		}
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
