/*   Copyright 2010 Juan Rada-Vilela

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "AreaCentroidAlgorithm.h"
#include "FuzzyOperation.h"
#include "LinguisticTerm.h"
namespace fl {

	std::string TriangulationAlgorithm::name() const {
		return "Triangulation";
	}

	flScalar TriangulationAlgorithm::area(const LinguisticTerm* term,
			int samples) const {
		flScalar x, y;
		return areaAndCentroid(term, x, y, samples);
	}

	void TriangulationAlgorithm::centroid(const LinguisticTerm* term,
			flScalar& x, flScalar& y, int samples) const {
		areaAndCentroid(term, x, y, samples);
	}

	flScalar TriangulationAlgorithm::areaAndCentroid(
			const LinguisticTerm* term, flScalar& centroid_x,
			flScalar& centroid_y, int samples) const {
		centroid_x = flScalar(0);
		centroid_y = flScalar(0);

		flScalar sum_area = flScalar(0.0);
		flScalar sum_a_x = flScalar(0.0);
		flScalar sum_a_y = flScalar(0.0);
		flScalar tmp_area = flScalar(0.0);

		flScalar range = term->maximum() - term->minimum();
		flScalar step_size = range / samples;

		int pi = 0;
		flScalar x[3];
		flScalar y[3];
		/*******
		 * first triangle should include (minimum, 0)
		 * fix requires y_is_0 to start false & pi to start at 1
		 * updated by rctaylor 02/08/11
		 ********/
		bool y_is_0 = false;
		x[0] = term->minimum();
		y[0] = term->membership(x[0]);
		x[1] = term->minimum();
		y[1] = 0;

		pi = 1;
		// end update 02/08/11

		int i = 1;
		//Triangulation:
		// rctaylor 02/08/11 - updated to <= because that is the only way x will ever equal
		// term->maximum (ie. term->maximum = term->minimum + samples * step_size)
		while (pi <= samples) {
			++i;
			if (y_is_0) {
				x[i % 3] = x[(i - 1) % 3];
				y[i % 3] = 0;
				++pi; // rctaylor 02/08/11 - since y_is_0 starts false, this moves here
			} else {
				x[i % 3] = term->minimum() + pi * step_size;
				y[i % 3] = term->membership(x[i % 3]);
			}

			y_is_0 = !y_is_0;

			//Area of triangle:
			// |Ax(By - Cy) + Bx(Cy - Ay) + Cx(Ay - By)| / 2
			// |x0(y1 - y2) + x1(y2 - y0) + x2(y0 - y1)| / 2
			tmp_area = FuzzyOperation::Absolute(x[0] * (y[1] - y[2]) + x[1]
					* (y[2] - y[0]) + x[2] * (y[0] - y[1])) / 2;
			//Centroid of a triangle:
			// x = (x0 + x1 + x2)/3; y = (y0 + y1 + y2)/3;
			sum_a_x += tmp_area * (x[0] + x[1] + x[2]) / 3;
			sum_a_y += tmp_area * (y[0] + y[1] + y[2]) / 3;

			sum_area += tmp_area;
		}
		centroid_x = sum_a_x / sum_area;
		centroid_y = sum_a_y / sum_area;
		return sum_area;
	}

	std::string TrapezoidalAlgorithm::name() const {
		return "Trapezoidal";
	}

	flScalar TrapezoidalAlgorithm::area(const LinguisticTerm* term, int samples) const {
		flScalar sum_area = 0.0;
		flScalar step_size = (term->maximum() - term->minimum()) / samples;
		flScalar step = term->minimum();
		flScalar mu, prev_mu = term->membership(step);

		for (int i = 0; i < samples; ++i) {
			step += step_size;
			mu = term->membership(step);

			sum_area += step_size * (mu + prev_mu);

			prev_mu = mu;
		}
		sum_area *= 0.5;

		return sum_area;
	}

	void TrapezoidalAlgorithm::centroid(const LinguisticTerm* term,
			flScalar& centroid_x, flScalar& centroid_y, int samples) const {
		areaAndCentroid(term, centroid_x, centroid_y, samples);
	}

	flScalar TrapezoidalAlgorithm::areaAndCentroid(const LinguisticTerm* term,
			flScalar& centroid_x, flScalar& centroid_y, int samples) const {
		flScalar sum_area = 0.0;
		flScalar step_size = (term->maximum() - term->minimum()) / samples;
		flScalar step = term->minimum();
		flScalar mu, prev_mu = term->membership(step);
		flScalar area, x, y;

		centroid_x = 0.0;
		centroid_y = 0.0;
		/*
		 *  centroid_x = a (h_1 + 2h_2)/3(h_1+h_2) ; h_1 = prev_mu; h_2 = mu
		 *  centroid_y = (h_1^2/(h_1+h_2) + h_2) * 1/3
		 */
		for (int i = 0; i <= samples ; ++i) {
			step += step_size;
			mu = term->membership(step);

			area = 0.5 * step_size * (prev_mu + mu);
			sum_area += area;

			x = ((step_size * (prev_mu + 2 * mu)) / (3.0 * (prev_mu + mu)))
					+ (step - step_size);
			y = ((prev_mu * prev_mu) / (prev_mu + mu) + mu) / 3.0;

			centroid_x += area * x;
			centroid_y += area * y;

			prev_mu = mu;
		}

		centroid_x /= sum_area;
		centroid_y /= sum_area;

		return sum_area;
	}
}
