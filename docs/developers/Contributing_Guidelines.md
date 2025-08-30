# Contributing Guidelines

Welcome to VCMI development! This guide will help you contribute effectively to the project.

## Getting Started

Before contributing, please:
1. Read the [Developer Quickstart Guide](Developer_Quickstart.md)
2. Check our [Coding Guidelines](Coding_Guidelines.md)
3. Join our [Discord community](https://discord.gg/chBT42V) for discussion

## Types of Contributions

### Bug Reports
- Use [GitHub Issues](https://github.com/vcmi/vcmi/issues) for bug reports
- Follow the [Bug Reporting Guidelines](../players/Bug_Reporting_Guidelines.md)
- Include minimal reproduction steps
- Provide system information and log files

### Feature Requests
- Discuss major features on Discord or forums first
- Create detailed GitHub issues with use cases
- Consider backward compatibility and existing code

### Code Contributions
- Start with issues labeled "good first issue"
- Fork the repository and create feature branches
- Follow coding standards and include tests
- Submit pull requests with clear descriptions

## Development Workflow

### 1. Fork and Clone
```bash
# Fork on GitHub, then:
git clone https://github.com/YOUR_USERNAME/vcmi.git
cd vcmi
git remote add upstream https://github.com/vcmi/vcmi.git
```

### 2. Create Feature Branch
```bash
git checkout develop
git pull upstream develop
git checkout -b feature/my-new-feature
```

### 3. Make Changes
- Follow [Coding Guidelines](Coding_Guidelines.md)
- Write tests for new functionality
- Update documentation as needed
- Test thoroughly on your platform

### 4. Commit Changes
```bash
# Use clear, descriptive commit messages
git add .
git commit -m "Add new battle spell effect system

- Implements customizable spell animations
- Adds configuration options for effect duration
- Includes unit tests for spell effect parsing
- Updates documentation for new API"
```

### 5. Push and Submit PR
```bash
git push origin feature/my-new-feature
# Create pull request on GitHub
```

## Pull Request Guidelines

### Before Submitting
- [ ] Code follows our style guidelines
- [ ] All tests pass locally
- [ ] New functionality includes tests
- [ ] Documentation is updated
- [ ] Commit messages are clear
- [ ] No merge conflicts with develop branch

### PR Description Template
```markdown
## Description
Brief description of changes and motivation.

## Type of Change
- [ ] Bug fix (non-breaking change which fixes an issue)
- [ ] New feature (non-breaking change which adds functionality)
- [ ] Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] Documentation update

## Testing
- [ ] Unit tests pass
- [ ] Manual testing completed
- [ ] Regression testing on affected features

## Screenshots (if applicable)
Include screenshots for UI changes.

## Checklist
- [ ] Code follows coding guidelines
- [ ] Self-review completed
- [ ] Documentation updated
- [ ] Tests added/updated
```

## Code Review Process

### What to Expect
1. **Automated checks** run on all PRs (CI/CD)
2. **Maintainer review** for code quality and design
3. **Community feedback** on significant changes
4. **Iterative improvements** based on feedback

### Review Criteria
- **Functionality**: Does it work as intended?
- **Design**: Is the approach sound and maintainable?
- **Testing**: Are there adequate tests?
- **Documentation**: Is it properly documented?
- **Style**: Does it follow our guidelines?

## Coding Standards

### Code Style
Follow our [Coding Guidelines](Coding_Guidelines.md):
- Use tabs for indentation
- CamelCase for classes, camelCase for variables
- Meaningful names and comments
- Modern C++ practices (C++17+)

### Architecture Principles
- **Client-Server separation**: UI logic vs game logic
- **Serializable state**: All game state must be serializable
- **Bonus system**: Use for all stat modifications
- **Type safety**: Prefer strong typing over raw values

### Testing Requirements
- **Unit tests** for all new algorithms and logic
- **Integration tests** for complex interactions
- **Manual testing** for UI and gameplay changes
- **Regression testing** for bug fixes

## Documentation Standards

### Code Documentation
```cpp
/**
 * Brief description of the class/function.
 * 
 * Detailed description explaining purpose, behavior,
 * and any important implementation details.
 * 
 * @param parameter Description of parameter
 * @return Description of return value
 * @throws ExceptionType When this might be thrown
 */
class MyClass
{
    /**
     * Brief method description.
     * More details if needed.
     */
    void myMethod(int parameter);
};
```

### User Documentation
- Update relevant `.md` files for user-facing changes
- Include examples and screenshots for complex features
- Test documentation accuracy

## Platform Considerations

### Cross-Platform Code
- Test on multiple platforms when possible
- Use CMake for build configuration
- Avoid platform-specific code without proper abstraction
- Consider endianness and architecture differences

### Mobile Platforms
- Consider touch interface for UI changes
- Test performance on resource-constrained devices
- Account for different screen sizes and orientations

## Specific Contribution Areas

### Game Logic
- Location: `lib/` directory
- Requires: Understanding of Heroes 3 mechanics
- Testing: Unit tests + gameplay verification

### User Interface
- Location: `client/` directory
- Requires: SDL2/Qt knowledge
- Testing: Visual verification + usability testing

### AI Development
- Location: `AI/` directory
- Requires: Understanding of game strategy
- Testing: AI vs AI battles, performance metrics

### Modding Support
- Location: `config/` and related systems
- Requires: JSON schema knowledge
- Testing: Mod loading and validation

### Map Editor
- Location: `mapeditor/` directory
- Requires: Qt knowledge
- Testing: Map creation and validation

## Community Guidelines

### Communication
- **Be respectful** and constructive in all interactions
- **Ask questions** when you're unsure about something
- **Help others** when you can share knowledge
- **Stay on topic** in discussions

### Decision Making
- **Discuss major changes** before implementation
- **Consider impact** on existing users and mods
- **Seek consensus** on controversial decisions
- **Document decisions** for future reference

## Getting Help

### Resources
- [Developer Quickstart](Developer_Quickstart.md) - Quick setup guide
- [API Reference](API_Reference.md) - Comprehensive API documentation
- [Testing Guide](Testing_and_Debugging.md) - Testing and debugging help

### Community Support
- **Discord**: Real-time chat and quick questions
- **Forums**: In-depth discussions and announcements
- **GitHub Issues**: Bug reports and feature requests
- **Code Review**: Learning opportunity through PR feedback

### Mentorship
- New contributors can request mentorship on Discord
- Experienced developers help with architecture questions
- Code review provides ongoing learning opportunities

## Recognition

### Contributors
- All contributors are acknowledged in AUTHORS.h
- Significant contributions are highlighted in release notes
- Active contributors may be invited to maintainer discussions

### Maintainers
- Long-term contributors may become maintainers
- Maintainers have additional responsibilities and privileges
- See [Maintainer Guidelines](../maintainers/Project_Infrastructure.md)

## Legal

### License
- All contributions are licensed under GPL v2+
- By contributing, you agree to license your work under these terms
- Include license headers in new files

### Copyright
- Contributors retain copyright to their contributions
- VCMI Team maintains project copyright
- Large contributions may require contributor agreement

## Thank You!

Thank you for contributing to VCMI! Your efforts help bring Heroes of Might & Magic III to new platforms and add exciting features for players worldwide.

Whether you're fixing a small bug, adding a major feature, or improving documentation, every contribution makes a difference. We appreciate your time and effort in making VCMI better for everyone.

Happy coding! 🎮✨