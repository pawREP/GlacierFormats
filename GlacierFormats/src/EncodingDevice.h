#pragma once

struct ID3D11Device;
struct ID3D11DeviceContext;

namespace GlacierFormats {

	class EncodingDevice {
	private:
		static ID3D11Device* device;
		static ID3D11DeviceContext* context;
	public:
		operator ID3D11Device* () const noexcept;
	};

}
